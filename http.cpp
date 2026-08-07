#include<iostream>
#include<sstream>
#include "http.h"

HttpRequest ParseRequest(const std::string& RequestedText){
HttpRequest request;
std::stringstream stream(RequestedText);
std::string line;


std::getline(stream,line);
if(!line.empty()&& line.back() =='\r'){
    line.pop_back();
}
std::stringstream firstLine(line);

firstLine>>request.method;
firstLine>>request.path;
firstLine>>request.version;

while(std::getline(stream,line)){
    if(!line.empty()&& line.back() == '\r'){
        line.pop_back();
    }
    if(line.empty()){
        break;
    }
    size_t colon = line.find(':');
    if(colon == std::string::npos){
        continue;;
    }
    std::string key = line.substr(0 , colon);
    std::string value = line.substr(colon +1);
    while(!value.empty()&& value.front()==' '){
        value.erase(value.begin());
    }
    request.headers[key] = value;
}

 auto it = request.headers.find("Transfer-Encoding");

    if (it != request.headers.end())
    {
        if (it->second == "chunked")
            request.chunked = true;
    }

  
    std::string body;

    if (request.chunked)
    {
        while (true)
        {
            std::getline(stream, line);

            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            int chunkSize = std::stoi(line, nullptr, 16);

            if (chunkSize == 0)
                break;

            std::string chunk(chunkSize, '\0');

            stream.read(&chunk[0], chunkSize);

            body += chunk;

            // Skip CRLF after each chunk
            stream.ignore(2);
        }
    }
    else
    {
        while (std::getline(stream, line))
        {
            body += line;

            if (!stream.eof())
                body += '\n';
        }
    }

    request.body = body;
return request;
}