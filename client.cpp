#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define CONTROL_PORT 21
#define BUF_SIZE 1024

void send_command(int sock,const std::string &command)
{
    send(sock,command.c_str(),command.length(),0);
}

std::string rec_response(int sock)
{
    char buffer[BUF_SIZE];
    int bytes = recv(sock,buffer,sizeof(buffer),0);

    if(bytes > 0)
    {
        buffer[bytes] = '\0';
        return std::string(buffer);
    }
    return "";
}

void list_files(int sock)
{
    send_command(sock,"list");
    std::string response = rec_response(sock);
   std::cout << "Server response: " << response << std::endl;
}

void download_file(int sock,const std::string &filename)
{
    send_command(sock, "RETR " + filename + "\r\n");
    std::string response = rec_response(sock);
      if (response.find("ERROR") != std::string::npos) {
        std::cout << "File not found." << std::endl;
        return;
    }

    std::ofstream file(filename,std::ios::binary);
    char buffer[BUF_SIZE];
    while(1)
    {
        int bytes = recv(sock,buffer,sizeof(buffer),0);
        if (bytes <= 0) {
            break;
        }
        file.write(buffer,bytes);
    }
    file.close();
    std::cout << "File downloaded successfully." << std::endl;
}

void upload_file(int sock,const std::string &filename)
{
   send_command(sock, "STOR " + filename + "\r\n");
   std::string response = rec_response(sock);
    if (response.find("ERROR") != std::string::npos) {
        std::cout << "Cannot upload file." << std::endl;
        return;
    }
    std::ifstream file(filename,std::ios::binary);
    char buffer[BUF_SIZE];
    while(file.read(buffer,sizeof(buffer)))
    {
        send(sock,buffer,file.gcount(),0);
    }
    file.close();
    std::cout << "File uploaded successfully." << std::endl;

}
int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    // char *message = "111";

    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    // if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
    //     perror("Invalid address or address not supported");
    //     close(sockfd);
    //     exit(EXIT_FAILURE);
    // }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
     std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::cout << "Connected to the server." << std::endl;
     std::string response = rec_response(sockfd);
    std::cout << "Server response: " << response << std::endl;

    list_files(sockfd);
    download_file(sockfd,"name");
    upload_file(sockfd,"name");

    // // 发送数据
    // if (send(sockfd, message, strlen(message), 0) == -1) {
    //     perror("Send failed");
    //     close(sockfd);
    //     exit(EXIT_FAILURE);
    // }

    // printf("Message sent to server: %s\n", message);

    // // 关闭连接
    close(sockfd);

    return 0;
}
