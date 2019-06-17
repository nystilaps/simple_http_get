#include "Socket.h"

#include <stdio.h> // printf, sprintf
#include <unistd.h> // read, write, close
#include <string.h> // memcpy, memset
#include <errno.h> // EAGAIN, EWOULDBLOCK, errno
#include <sys/socket.h> // socket, connect
#include <netinet/in.h> // struct sockaddr_in, struct sockaddr
#include <netdb.h> // struct hostent, gethostbyname 

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>


Socket::Socket(int domain, int type, int protocol) {
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        throw std::runtime_error("Error opening socket");
    }

    {
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
            perror("Error setting socket recv timeout");
            close(sockfd); // RAII works only on constructor end
            throw std::runtime_error("Error setting socket read timeout");
        }

        // Do not set send timeout to avoid select in connect(...)
    }
}

double Socket::getTimeoutSeconds() {
    return timeout;
}

// socket file descriptor
int Socket::fd() {
    return sockfd;
}

bool Socket::setTimeoutSeconds(int seconds) {
    timeout = seconds;
    return true;
}

Socket::~Socket() {
    if (sockfd < 0)
        return;
    close(sockfd);
}

bool Socket::connect(const std::string &host, int port) {
    hostent *server;

    // lookup the ip address
    server = gethostbyname(host.c_str());
    if (server == NULL) {
        std::cerr << "ERROR, can't find host " <<  host << std::endl;
        perror(host.c_str());
        return false;
    }

    // fill in the structure
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // connect the socket
    if (::connect(sockfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::stringstream ss;
        ss << "ERROR connecting socket to " << host << ":" << port;
        perror(ss.str().c_str());
        return false;
    }

    return true;
}

bool operator << (Socket &s, const std::string &msg)
{
	using namespace std::chrono;

    const char *message = msg.c_str();
    ssize_t total = msg.size();
    ssize_t sent = 0;

    auto start = steady_clock::now();

    do {
        int bytes = send(s.fd(), message+sent, total-sent, MSG_DONTWAIT);

        if (bytes < 0) {
            if (errno == EAGAIN  || errno == EWOULDBLOCK) {
                auto end = steady_clock::now();
                double secondsPassed =
                	duration_cast<microseconds>(end - start).count() / 1000000.0;
                if (secondsPassed > s.getTimeoutSeconds())
                    throw std::runtime_error("Timout sending data!");
                continue;
            }
            perror("ERROR writing message to socket");
            throw std::runtime_error("Error, writing data to socket!");
        }
        else if (bytes == 0)
            break;

        start = steady_clock::now(); // last time we wrote data to socket

        sent += bytes;
    } while (sent < total);

    return (sent == total);
}

// Read binary data from socket
size_t operator >> (Socket &s, std::vector<char> &ret)
{
	using namespace std::chrono;

	if (ret.size() == 0)
		throw std::runtime_error("Error: must read from socket into non-zero size vector!");

    char *response = ret.data();
    const ssize_t total = ret.size();
    ssize_t received = 0;

    auto start = steady_clock::now();

    do {
        ssize_t bytes = recv(s.fd(), response+received, total-received, 0);

        if (bytes < 0) {
            if (errno == EAGAIN  || errno == EWOULDBLOCK) {
                auto end = steady_clock::now();
                double secondsPassed =
                	duration_cast<microseconds>(end - start).count() / 1000000.0;
                if (secondsPassed > s.getTimeoutSeconds())
                    throw std::runtime_error("Timout reading data!");
                continue;
            }
            throw std::runtime_error("ERROR reading response from socket!");
        }
        else if (bytes == 0)
            break;

        start = steady_clock::now(); // last time we got data from socket
        received += bytes;

    } while (received < total);

    ret.resize(received);

    return received;
}
