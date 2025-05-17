#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>


//#define PORT 8080
#define CONTROL_PORT 21
#define BUF_SIZE 1024

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return ""; // no content
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}


void send_response(int client_socket,const std::string &response)
{
    send(client_socket,response.c_str(),response.length(),0);
}

void handle_client(int client_socket)
{
    char buffer[BUF_SIZE];
    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        int bytes = recv(client_socket,buffer,sizeof(buffer),0);

        if(bytes <= 0 )
        {
            break;
        }

        std::string command(buffer);

          command = trim(command);
          
        std::stringstream response;

        if(command == " LIST")
        {

            DIR *dir = opendir(".");
            if(dir)
            {
                struct dirent *entry;
                while((entry = readdir(dir))!=nullptr)
                {
                    response << entry->d_name <<"\r\n";
                }
                closedir(dir);
                send_response(client_socket,response.str());
            }else{
                send_response(client_socket,"ERROR1");
            }
        }else if(command.rfind("RETR ", 0) == 0)
        {
            std::string filename = command.substr(5);
            std::ifstream file(filename,std::ios::binary);
            if(file.is_open())
            {
                while(!file.eof())
                {
                    file.read(buffer,sizeof(buffer));
                    send(client_socket,buffer,file.gcount(),0);
                }
                file.close();
            }else{
                send_response(client_socket,"ERROR2");
            }
        }else if(command.rfind("STOR ", 0) == 0)
        {
            std::string filename = command.substr(5);
            std::ofstream file(filename,std::ios::binary);
            if(file.is_open())
            {
                while(true)
                {
                    int bytes = recv(client_socket,buffer,sizeof(buffer),0);

                    if(bytes <= 0)
                    {
                        break;
                    }
                    file.write(buffer,bytes);
                }
                file.close();
                send_response(client_socket, "200 OK\r\n");
            }else {
                send_response(client_socket, "ERROR3: Unable to open file.\r\n");
            }
        }else {
            send_response(client_socket, "ERROR: Unknown command.\r\n");
        }
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
     socklen_t client_len = sizeof(client_addr);
    //char buffer[BUF_SIZE];

    // 创建 TCP 套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    server_addr.sin_port =    server_addr.sin_port = htons(CONTROL_PORT);       // 设置端口号

    // 绑定套接字到指定的端口和 IP 地址
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_fd);
        return 1;
    }

    // 开始监听客户端连接
    if (listen(server_fd, 3) == -1) {
        perror("Listen failed");
        close(server_fd);
       return 1;
    }
   std::cout << "FTP Server listening on port " << CONTROL_PORT << "..." << std::endl;

   while(1)
   {
    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if(client_socket < 0)
    {
        std::cerr << "Failed to accept client connection." << std::endl;
            continue;
    }

     std::cout << "Client connected." << std::endl;
     send_response(client_socket,"220 Welcome to the FTP server\r\n");
     handle_client(client_socket);
     close(client_socket);
       std::cout << "Client disconnected." << std::endl;

   }
    // 接受客户端连接
    // if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
    //     perror("Accept failed");
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }

    // printf("Client connected\n");

    // // 读取客户端发送的数据
    // int bytes_received = recv(client_fd, buffer, BUF_SIZE, 0);
    // if (bytes_received == -1) {
    //     perror("Recv failed");
    //     close(client_fd);
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }
    
    // buffer[bytes_received] = '\0';  // 添加字符串结束符
    // printf("Received from client: %s\n", buffer);

    // // 关闭连接
    // close(client_fd);
    close(server_fd);

    return 0;
}
