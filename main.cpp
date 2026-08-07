#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "http.h"
int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return -1;
    }

    int flags = fcntl(server_fd, F_GETFL, 0);

    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        close(server_fd);
        return -1;
    }

    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        close(server_fd);
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8000);

    if (bind(server_fd,
             (sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    std::cout << "Binding successful\n";

    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return -1;
    }

    std::cout << "Listening...\n";

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(server_fd);
        return -1;
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd,
                  EPOLL_CTL_ADD,
                  server_fd,
                  &event) == -1)
    {
        perror("epoll_ctl ADD server");
        close(epoll_fd);
        close(server_fd);
        return -1;
    }

    epoll_event events[60];

    while (true)
    {
        int ready = epoll_wait(epoll_fd,
                               events,
                               60,
                               -1);

        if (ready == -1)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < ready; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                while (true)
                {
                    sockaddr_in client;
                    socklen_t client_size = sizeof(client);

                    int client_fd =
                        accept(server_fd,
                               (sockaddr *)&client,
                               &client_size);

                    if (client_fd == -1)
                    {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                            break;

                        perror("accept");
                        break;
                    }

                    std::cout << "Client Connected : "
                              << client_fd
                              << std::endl;

                    int client_flags =
                        fcntl(client_fd,
                              F_GETFL,
                              0);

                    if (client_flags == -1)
                    {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }

                    if (fcntl(client_fd,
                              F_SETFL,
                              client_flags | O_NONBLOCK) == -1)
                    {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }

                    epoll_event client_event{};
                    client_event.events =
                        EPOLLIN |
                        EPOLLET |
                        EPOLLONESHOT;

                    client_event.data.fd =
                        client_fd;

                    if (epoll_ctl(epoll_fd,
                                  EPOLL_CTL_ADD,
                                  client_fd,
                                  &client_event) == -1)
                    {
                        perror("EPOLL_CTL_ADD");
                        close(client_fd);
                    }
                }
            }
            else
            {
                while (true)
                {
                    char buffer[4096];

                    int bytes =
                        recv(events[i].data.fd,
                             buffer,
                             sizeof(buffer) - 1,
                             0);

                    if (bytes > 0)
                    {
                        HttpRequest request = ParseRequest(buffer);
                        buffer[bytes] = '\0';
                        
                        std::cout
                            << "\n========== HTTP REQUEST ==========\n";

                        std::cout << request.method<<request.path<<" "<<request.version << std::endl;
                        for(const auto& header :request.headers){
                            std::cout<<header.first<<" : "<<header.second<<std::endl;
                        }
                      
                        std::cout<<request.body<<std::endl;
                    }
                    else if (bytes == 0)
                    {
                        std::cout
                            << "Client disconnected\n";

                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  events[i].data.fd,
                                  nullptr);

                        close(events[i].data.fd);

                        break;
                    }
                    else
                    {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                        {
                            epoll_event client_event{};

                            client_event.events =
                                EPOLLIN |
                                EPOLLET |
                                EPOLLONESHOT;

                            client_event.data.fd =
                                events[i].data.fd;

                            if (epoll_ctl(epoll_fd,
                                          EPOLL_CTL_MOD,
                                          events[i].data.fd,
                                          &client_event) == -1)
                            {
                                perror("EPOLL_CTL_MOD");

                                close(events[i].data.fd);
                            }

                            break;
                        }

                        perror("recv");

                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  events[i].data.fd,
                                  nullptr);

                        close(events[i].data.fd);

                        break;
                    }
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);

    return 0;
}