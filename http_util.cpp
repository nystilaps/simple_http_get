#include "http_util.h"

#include <sstream>
#include <regex>
#include <algorithm>
#include <string> 

HttpResponse::HttpResponse(std::string &s)
{
    std::istringstream iss(s);
    std::string l;
    int i = 0;

    while (getline(iss, l, '\n'))
    {
        auto pos = l.find(' ');
        if (pos != l.npos && pos != 0)
        {
            if (i == 0) {
                code = l.substr(pos + 1); // after first ' '
                if (code.length())
                	code.pop_back(); // remove '\r'

            } else {
                std::string value = l.substr(pos + 1);
                std::string name = l.substr(0, pos);

                if (name.length())
                    name.pop_back(); // remove ':'
                if (value.length())
                    value.pop_back(); // remove '\r'

                // Lower case field name
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                fields[name] = value;
            }
        }
        i++;
    }
}

// Read HTTP response header
size_t operator >> (Socket &s, HttpResponse &resp)
{
	std::string res;
    std::vector<char> response(1, 0);
    ssize_t received = 0;

    const std::string ending = "\r\n\r\n";

    while (int bytes = (s >> response))
    {
        received += bytes;
        res.push_back(response[0]);
        response.resize(1);

        if ( (res.length() >= ending.length())
             && (res.compare(res.length() - ending.length(), ending.length()
                           , ending) == 0) ) 
        {
        	resp = HttpResponse(res);
            return true;
        }
    }

    return false;
}

std::string formatGETHeader(const std::string &host
    , const int port
    , const std::string &path)
{
    std::stringstream ss;
    ss << "GET " << path << " HTTP/1.0\r\n"
        "Host: " << host << ":" << port << "\r\n"
        "Content-type: text/plain\r\n"
        "Content-length: 0\r\n"
        "\r\n"
        "\r\n";

    return ss.str();
}

ParsedUrl parseUrl(const std::string &url)
{
    ParsedUrl purl;

    {
        // URL parsing regexp by Tim Berners-Lee ("//" moved from group 3 to group 1):
        static std::regex rex( R"asdf(^(([^:/?#]+)://)?(([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)asdf" ); //"([Hh][Tt][Tt][Pp]:\\)?()");
        //                             12              3  4          5       6  7        8 9
        //
        // The numbers in the second line above are only to assist readability;
        // they indicate the reference points for each subexpression
        // 
        // Example url:
        // http://www.ics.uci.edu/pub/ietf/uri/#Related
        //
        // results in the following subexpression matches:
        //
        // $1 = http:
        // $2 = http
        // $3 = //www.ics.uci.edu
        // $4 = www.ics.uci.edu
        // $5 = /pub/ietf/uri/
        // $6 = <parameters string with '?' (not present in example)>
        // $7 = <parameters string without '?' (not present in example)>
        // $8 = #Related
        // $9 = Related

        std::smatch m;
        auto match = std::regex_match(url, m, rex);

        if (!match)
            throw std::runtime_error("URL parse failed!");

        purl = ParsedUrl{ m[2], m[4], 80
        	, std::string(m[5]) + std::string(m[6]) + std::string(m[8]) };
    }

    if (purl.protocol.empty())
        purl.protocol = "http";

    std::transform(purl.protocol.begin(), purl.protocol.end(), purl.protocol.begin(), ::tolower);
    std::transform(purl.host.begin(), purl.host.end(), purl.host.begin(), ::tolower);

    {
        // Parses host into host:port
        static std::regex hostRex( R"asdf((.*)(:(\d*?))$)asdf" );
        // Group positions                1   2 3

        std::smatch m;
        auto match = std::regex_match(purl.host, m, hostRex);

        if (!match) {
            return purl;
        }

        purl.host = m[1];
        purl.port = std::stoi(m[3]);
    }

    return purl;
}

std::string formatTime(double time)
{
    std::string timeDimentionality = "sec";

    if (time > 60) {
        time /= 60.;
        timeDimentionality = "min";

	    if (time > 60) {
	        time /= 60.;
	        timeDimentionality = "hours";

		    if (time > 24) {
		        time /= 24.;
		        timeDimentionality = "days";
		    }
	    }
    }

    std::stringstream ss;
    ss << time << " " << timeDimentionality;

    return ss.str();
}

std::string formatSize(double s)
{
    std::string dimentionality = "b";

    if (s > 1024) {
        s /= 1024.0;
        dimentionality = "Kb";

	    if (s > 1024) {
	        s /= 1024.0;
	        dimentionality = "Mb";

		    if (s > 1024) {
		        s /= 1024.0;
		        dimentionality = "Gb";
		    }
	    }
    }

    std::stringstream ss;
    ss << s << " " << dimentionality;
    
    return ss.str();
}
