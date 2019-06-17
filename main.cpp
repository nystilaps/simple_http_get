#include "Socket.h"
#include "http_util.h"
#include "throw.h"

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>


int downloadUrl(const std::string &url, int redirectLevel = 0);
int processResponse(const HttpResponse &resp, const ParsedUrl &purl, int redirectLevel);
int downloadFile(Socket &s, ssize_t expectedBytes);


int main(int argc, char *argv[])
try
{
    if (argc != 2) {
        std::cerr << "Usage: ./http_get 'url' > output_file" << std::endl;
        return 101;
    }

    return downloadUrl(argv[1]);
}
catch(std::exception &e)
{
    std::cerr << "Error occurred: " << e.what() << std::endl;
    return 100;
}
//-----------------


int processResponse(const HttpResponse &resp, const ParsedUrl &purl, int redirectLevel)
{
    ssize_t expectedBytes = -1;

    if (resp.code != "200 OK")
    {
        std::cerr << "Got response: " << resp.code << std::endl;

        const std::vector<std::string> redirectCodes =
            { "300 ", "301 ", "302 ", "303 ", "307 ", "308 " };

        for (const auto &code: redirectCodes) {
            if (!resp.code.compare(0, code.size(), code)) {
                std::cerr << "Redirect code" << std::endl;

                if (resp.fields.count("location"))
                {
                    std::string location = resp.fields.at("location");
                    if (!location.empty() && (location[0] == '/')) {
                        std::stringstream ss;
                        ss << purl.protocol << "://"
                            << purl.host << ":" << purl.port
                            << location;
                        location = ss.str();
                    }

                    std::cerr << "Redirected to URL: " << location << std::endl;
                    return downloadUrl(location, redirectLevel + 1);
                }

                Throw<>() << "Error: Got redirect without specified location!";
            }
        }

        Throw<>() << "Got non-Ok non-redirect response: " << resp.code;
    }

    if (resp.fields.count("content-length")) {
        try {
            expectedBytes = std::stoll(resp.fields.at("content-length"));
        } catch (std::exception &e) {
            Throw<>() << "Error converting 'content-length' to number: "
                << "'" << resp.fields.at("content-length") << "'";
        }
    }

    return expectedBytes;
}

int downloadFile(Socket &s, ssize_t expectedBytes)
{
    std::cerr << "Downloading file..." << std::endl;
    ssize_t gotBytes = 0;
    {
        using namespace std::chrono;

        auto start = steady_clock::now();
        auto end = steady_clock::now();
        auto downloadStart = steady_clock::now();

        const size_t minChunkSize = 1024;
        const size_t maxChunkSize = 10*1024*1024;
        size_t chunkSize = 100 * minChunkSize;
        std::vector<char> data(chunkSize, 0);

        while (size_t bytes = (s >> data))
        {
            gotBytes += bytes;
            std::cout.write(data.data(), bytes);

            auto now = steady_clock::now();
            double secondsOnLastChunk =
                duration_cast<microseconds>(now - end).count() / 1000000.0;
            if (secondsOnLastChunk < 0.5) {
                chunkSize *= 1.5;
                if (chunkSize > maxChunkSize)
                    chunkSize = maxChunkSize;
            }
            else {
                chunkSize *= 0.8;
                if (chunkSize < minChunkSize)
                    chunkSize = minChunkSize;
            }

            end = now;
            double secondsPassed =
                duration_cast<microseconds>(end - start).count() / 1000000.0;

            if (secondsPassed > 1) {
                start = steady_clock::now();

                double secondsDownloaded =
                    duration_cast<microseconds>(end - downloadStart).count() / 1000000.0;

                double speed = gotBytes / (secondsDownloaded + 0.000001);
                std::string speedDimentionality = "B/sec";

                std::cerr << "Progress: " << formatSize(gotBytes)
                    << ", "<< formatSize(speed) << "/sec"
                    << ", " << formatTime(secondsDownloaded) << " passed";

                if (expectedBytes > 0) {
                    double percent = 100 * gotBytes/(double)expectedBytes;

                    double timeLeft = (expectedBytes - gotBytes) / speed;
                    std::string timeDimentionality = "sec";

                    std::cerr << ", " << percent << "%"
                        << ", " << formatTime(timeLeft) << " left"
                        << "                                             ";
                }
                std::cerr << '\r';
            }

            data.resize(chunkSize);
        }

        std::cerr << "Download finished.                                  "
            << "                                                          "
            << std::endl;

        {
            auto end = steady_clock::now();
            double secondsDownloaded =
                duration_cast<microseconds>(end - downloadStart).count() / 1000000.0;
            std::cerr << "Downloaded " << formatSize(gotBytes) 
                << " in " << formatTime(secondsDownloaded) << "." << std::endl;
        }
    }

    if (expectedBytes >= 0) {
        if (gotBytes == expectedBytes)
            std::cerr << "File size matches server size announcement." << std::endl;
        else {
            std::cerr << "Error: Downloaded file size does not match server announcement!"
                << std::endl;
            std::cerr << "Server announced " << expectedBytes << " bytes"
                << " got " << gotBytes << " bytes." << std::endl;
            return 102;
        }
    }
    else
        std::cerr << "File size was not announced by server. Couldn't check file size."
            << std::endl;

    return 0;
}

int downloadUrl(const std::string &url, int redirectLevel)
{
    if (redirectLevel > 10)
        Throw<>() << "Too deep redirect level!";

    auto purl = parseUrl(url);

    if (purl.protocol != "http")
        Throw<>() << "Protocol in URL is " << purl.protocol << ", not http!";

    const std::string message = formatGETHeader(purl.host, purl.port, purl.path);

    Socket s(AF_INET, SOCK_STREAM, 0);

    if (!s.setTimeoutSeconds(10))
        Throw<>() << "Set socket timeout failed";

    std::cerr << "Looking up host: " << purl.host << std::endl;
    if (!s.connect(purl.host, purl.port))
        Throw<>() << "Socket connect failed to port " << purl.port << "!";
    std::cerr << "Host found and connected." << std::endl;

    if (!(s << message))
        Throw<>() << "Couldn't send GET request!";

    HttpResponse response;
    if (!(s >> response))
        Throw<>() << "HTTP response header read failed!";

    ssize_t expectedBytes = processResponse(response, purl, redirectLevel);

    return downloadFile(s, expectedBytes);
}
