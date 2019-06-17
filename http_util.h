#pragma once

#include "Socket.h"

#include <string>
#include <map>

// Parsed HTTP response
struct HttpResponse
{
    std::string code;
    std::map<std::string, std::string> fields;

    explicit HttpResponse(std::string &s);
	HttpResponse() = default;
    HttpResponse(HttpResponse &&) = default;
    HttpResponse& operator = (const HttpResponse &) = default;
    HttpResponse& operator = (HttpResponse &&) = default;
};

// Read HTTP response header
size_t operator >> (Socket &s, HttpResponse &res);

// format HTTP GET request header
std::string formatGETHeader(const std::string &host
    , const int port
    , const std::string &path);

// Parsed URL
struct ParsedUrl {
    std::string protocol;
    std::string host;
    int port;
    std::string path;
};
ParsedUrl parseUrl(const std::string &url);

// Format time as sec, min, hours, days
std::string formatTime(double time);

// Format file size as b, Kb, Mb, Gb
std::string formatSize(double s);
