#pragma once

#include <sys/socket.h> // AF_INET, SOCK_STREAM

#include <string>
#include <vector>

class Socket
{
    int sockfd;
    double timeout = 10;

    // socket file descriptor for friends
    int fd();

public:
    Socket(int domain, int type, int protocol);
    ~Socket();
    double getTimeoutSeconds();
    bool setTimeoutSeconds(int seconds);
    bool connect(const std::string &host, int port);

	friend size_t operator >> (Socket &s, std::vector<char> &ret);
	friend bool operator << (Socket &s, const std::string &msg);    
};
