#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    // 创建客户端套接字
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 本地地址，若是远程服务器可替换为远程IP

    // 连接到服务器
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 发送数据
    std::string message;
    std::cout << "输入ovo" << std::endl;
    std::cin >> message;

    // 修改这里，将 std::string 转换为 C 风格字符串
    if (send(client_fd, message.c_str(), message.size(), 0) == -1) {
        perror("send");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Message sent: " << message << std::endl;

    // 关闭套接字
    close(client_fd);

    return 0;
}
