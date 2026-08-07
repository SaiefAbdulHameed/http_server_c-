#ifndef HTTP_H
#define HTTP_H
#include<string>
#include<unordered_map>

struct HttpRequest{
std::string method;
std::string path;
std::string version;
std::unordered_map<std::string,std::string> headers;
std::string body;
bool chunked = false;
};

HttpRequest ParseRequest(const std::string& requestedText);

#endif