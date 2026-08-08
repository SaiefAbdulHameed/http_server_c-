#include<cstring>
#include<iostream>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/sendfile.h>
#include<sys/socket.h>
#include<fcntl.h>
#include "response.h"

#include<string>

void SendFileResponse(int client_fd,const std::string& path){

    std::string filePath;
    if(path =="/"){
        filePath = "www/index.html";
    }
    else{
        filePath = "www"+path;
    }

    int file_fd = open(filePath.c_str(),O_RDONLY);
    std::cout << "Trying to open: " << filePath << std::endl;
    if(file_fd ==-1){
        const char* response = 
        "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 13\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "404 Not Found";

        send(client_fd,response,strlen(response),0);
        return ;    
    }

    struct stat FileStats;
    fstat(file_fd,&FileStats);
    std::string header = 
     "HTTP/1.1 200 OK\r\n"
        "Content-Length: " +
        std::to_string(FileStats.st_size) +
        "\r\n"
        "Content-Type: text/html\r\n"
        "\r\n";

        send(client_fd,
         header.c_str(),
         header.size(),
         0);

    off_t offset = 0;

    sendfile(client_fd,
             file_fd,
             &offset,
             FileStats.st_size);

    close(file_fd);

}