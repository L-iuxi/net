#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 10

// 设置非阻塞模式
void setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 创建epoll实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 创建一个监听套接字
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        close(epfd);
        exit(EXIT_FAILURE);
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        close(epfd);
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, 5) == -1) {
        perror("listen");
        close(listen_fd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    // 设置监听套接字为非阻塞
    setnonblocking(listen_fd);

    struct epoll_event ev; // 将监听套接字添加到epoll实例
    ev.events = EPOLLIN;//文件描述符可以读取数据。
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {//是否有可读事件（即新的连接请求）
        perror("epoll_ctl: listen_fd");
        close(listen_fd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    
    struct epoll_event events[MAX_EVENTS];//一个新的epoll_event 结构体数组，用来存放事件
    while (true) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        //epoll_wait 等待 epoll 实例中的事件。当有事件发生时，它会将事件存储在 events 数组中
        
        if (nfds == -1) {//nfd返回发生的事件数量
            perror("epoll_wait");
            close(listen_fd);
            close(epfd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listen_fd) {
                // 新连接到来
                int client_fd = accept(listen_fd, nullptr, nullptr);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                // 设置客户端套接字为非阻塞
                setnonblocking(client_fd);

                // 将客户端套接字添加到epoll实例
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                    continue;
                }

                std::cout << "有新的连接" << std::endl;
            } else {
                // 处理客户端事件
                int client_fd = events[i].data.fd;
                char buffer[512];
                int bytes_read = read(client_fd, buffer, sizeof(buffer));
                if (bytes_read == -1) {
                    perror("read");
                    close(client_fd);
                } else if (bytes_read == 0) {
                    std::cout << "Client disconnected" << std::endl;
                    close(client_fd);
                } else {
                 
                    std::cout << "接受到：" << buffer << std::endl;
                   
                }
            }
        }
    }

    // 关闭文件描述符
    close(listen_fd);
    close(epfd);

    return 0;
}
