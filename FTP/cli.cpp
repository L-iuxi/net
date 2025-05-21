#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sstream>

#define CONTROL_PORT 21
void send_data(int sockfd, const std::string& data);
void command_part(const std:: string &cmd,int sock)
{
    if(cmd == "LIST")
    {
        //std::cout << "now command is LIST" << std::endl;
        std::string directory;
        std::cout << "enter thr dir" << std::endl;
        std::getline(std::cin, directory);
        std::ostringstream list_command;
    list_command << "LIST " << directory << "\r\n";
    send_data(sock, list_command.str());
        // send_data(control_sock, "CWD " + directory + "\r\n");
        // response = receive_data(control_sock);
        // //std::cout << "Server response: " << response << std::endl;
        // send_data(sock,"LIST\r\n");
    }else if(cmd == "STOR"){
        std::cout << "now command is STOR" << std::endl;
    }else if(cmd == "RETR")
    {
        std::cout << "now command is RETR" << std::endl;
    }else{
        std::cout << "other command" << std::endl;
    }
    
}
//控制线程
int create_control_socket(const char* server_ip) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONTROL_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Connection failed" << std::endl;
        exit(1);
    }

    return sockfd;
}


void send_data(int sockfd, const std::string& data) {
    send(sockfd, data.c_str(), data.length(), 0);
}


std::string receive_data(int sockfd) {
    char buffer[1024];
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }
    return "";
}

//提取PASV响应中的数据端口
int extract_data_port(const std::string& pasv_response) {
    size_t start_pos = pasv_response.find('(');
    size_t end_pos = pasv_response.find(')');
    if (start_pos != std::string::npos && end_pos != std::string::npos) {
        std::string port_info = pasv_response.substr(start_pos + 1, end_pos - start_pos - 1);
        int a, b, c, d, e, f;
        sscanf(port_info.c_str(), "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &e, &f);
        return (e * 256) + f;
    }
    return -1;
}

// 数据线程
int create_data_socket(const char* server_ip, int data_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Data socket creation failed" << std::endl;
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Data connection failed" << std::endl;
        exit(1);
    }

    return sockfd;
}


void run_client(const char* server_ip) {
    int control_sock = create_control_socket(server_ip);
    send_data(control_sock, "PASV\r\n");

    // 接收服务器响应
    std::string response = receive_data(control_sock);
    std::cout << "Server response: " << response << std::endl;

    
    int data_port = extract_data_port(response);
    if (data_port == -1) {
        std::cerr << "Failed to extract data port" << std::endl;
        return;
    }
    std::cout << "Connecting to data port: " << data_port << std::endl;

    
     int data_sock = create_data_socket(server_ip, data_port);

     std ::string cmd;
     std::cout << "Please enter command2: ";
        std::getline(std::cin, cmd);
    command_part(cmd,control_sock);



   
     send_data(data_sock, "1234567\r\n");

    // 接收服务器的响应，确认发送成功
    response = receive_data(data_sock);
    std::cout << "Server response to data transfer: " << response << std::endl;
     std::cout << response << std::endl;
    // 关闭连接
    close(data_sock);
    close(control_sock);
}


int main() {
    const char* server_ip = "127.0.0.1";
    std :: string cmd;
    std :: cout << "please enter command:" <<std::endl;
    std::getline(std::cin, cmd);
    if(cmd == "PASV")
    {
        std :: cout <<  "pasv" <<std::endl;
        run_client(server_ip);
    }else{
         std :: cout << "not pasv "<<std::endl;
    }

    return 0;
}
