#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>


#define CONTROL_PORT 21


int create_control_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Binding failed" << std::endl;
        exit(1);
    }

    if (listen(sockfd, 1) == -1) {
        std::cerr << "Listen failed" << std::endl;
        exit(1);
    }

    return sockfd;
}
void send_data(int sockfd, const std::string& data) {
    send(sockfd, data.c_str(), data.length(), 0);
}
// accept
int accept_client_connection(int control_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(control_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock == -1) {
        std::cerr << "Client connection failed" << std::endl;
        return -1;
    }
    return client_sock;
}

// 接收客户端命令
ssize_t receive_command(int client_sock, char* buffer, size_t buffer_size) {
    ssize_t bytes_received = recv(client_sock, buffer, buffer_size - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    return bytes_received;
}

// 函数：处理PASV命令
void handle_pasv(int control_sock, int data_sock) {
    // 创建新的套接字并绑定一个随机端口
    int data_port = 12345;  // 示例端口号
    struct sockaddr_in data_addr;
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(data_port);

    if (bind(data_sock, (struct sockaddr*)&data_addr, sizeof(data_addr)) == -1) {
        std::cerr << "Data binding failed" << std::endl;
        exit(1);
    }

    if (listen(data_sock, 1) == -1) {
        std::cerr << "Data listen failed" << std::endl;
        exit(1);
    }

    // 向客户端发送PASV响应，告知客户端使用的数据连接端口
    std::string response = "227 Entering Passive Mode (127,0,0,1,48,57)\r\n";  // 示例IP和端口12345
    send(control_sock, response.c_str(), response.length(), 0);
}

// 接收数据
std::string receive_data(int sockfd) {
    char buffer[1024];
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }
    return "";
}
void handle_list(int data_sock,const std::string &dir_name)
{
    DIR *dir;
    struct dirent *entry;
    std::cout << "r1" << std::endl;
    dir = opendir(dir_name.c_str());
    entry = readdir(dir);
    std::cout << "r2" << std::endl;
    
    if(dir != NULL)
    {
         std::cout << "right dir" << std::endl;
         std::string file_list;
        while (entry != NULL) {
            std::cout << "1" << std::endl;
            file_list += entry->d_name;
            file_list += "\r\n";
            entry = readdir(dir);
        }
        std::cout << "file_list is" <<file_list<< std::endl;
        closedir(dir);
        std::cout << "right dir" << std::endl;
        
        struct sockaddr_in data_conn_addr;
        socklen_t data_conn_len = sizeof(data_conn_addr);
       // int data_client_sock = accept(data_sock, (struct sockaddr*)&data_client_addr, &data_client_len);

        int data_conn = accept(data_sock, (struct sockaddr*)&data_conn_addr, &data_conn_len);
        std::cout << "3" << std::endl;
        if (data_conn != -1) {
            std::cout << "right data_conn" << std::endl;
            send_data(data_sock, file_list);
            std::cout << "Sent file list to client." << std::endl;
            close(data_conn);
        }else{
            std::cout << "wrong data_conn" << std::endl;
        }
    }else{
        std::cout << "wrong dir" << std::endl;
    }
    
}
// 客户端命令解析
void handle_client(int client_sock) {
    char buffer[1024];

   
    ssize_t bytes_received = receive_command(client_sock, buffer, sizeof(buffer));
    if (bytes_received > 0) {
        std::cout << "Received command: " << buffer << std::endl;
    }

    if (strncmp(buffer, "PASV", 4) == 0) {
        std::cout << "Entering PASV mode..." << std::endl;
        int data_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sock == -1) {
            std::cerr << "Data socket creation failed" << std::endl;
            return;
        }

        handle_pasv(client_sock, data_sock);
        struct sockaddr_in data_client_addr;
        socklen_t data_client_len = sizeof(data_client_addr);
        int data_client_sock = accept(data_sock, (struct sockaddr*)&data_client_addr, &data_client_len);
      
         receive_data(data_sock);
        // close(data_client_sock);
        // close(data_sock);
        
        std::string command = receive_data(client_sock);
        //std::cout << "收到命令: " << command << std::endl;
       
       
        if (strncmp(command.c_str(), "LIST", 4) == 0) {
         std::cout << "LIST command received" << std::endl;

       
        std::string dir_name = command.substr(5);
        dir_name = dir_name.substr(0, dir_name.find("\r\n")); // 去除末尾的\r\n
        std::cout << "目录: " << dir_name << std::endl;

        handle_list(data_sock, dir_name); // 处理目录列表
        //close(data_sock);
        }
        close(data_client_sock);
        close(data_sock);
        
    }
  

    close(client_sock);
}

int main() {
   
    int control_sock = create_control_socket();
    std::cout << "FTP server running on port " << CONTROL_PORT << std::endl;

    
    // ThreadPool threadPool(MAX_THREADS);
    while (true) {
        int client_sock = accept_client_connection(control_sock);
        handle_client(client_sock);
    }

 
    close(control_sock);
    return 0;
}
