#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>  
#include <algorithm> 


#define CONTROL_PORT 21
#define BUF_SIZE 1024


void list_files(int sock);
void upload_file(int sock,const std::string &filename);
void download_file(int sock,const std::string &filename);
void handle_pasv(int sock);
void command_parse(const std::string &input,int sockfd,int e)
{

    if (input.find("LIST") != std::string::npos) {
        std::cout << "Found LIST command!" << std::endl;
        // list = 1;
        list_files(sockfd);
    } else if(input.find("RETR") != std::string::npos)
    {
        //retr = 1;
        std::string command, filename;
        std::istringstream stream(input);  
        stream >> command >> filename; 
        //std::cout << "filename is :" <<filename << std::endl;  

        if (!filename.empty()) 
        {
            //std::cout << "Found RETR command with filename: " << filename << std::endl;
           
           
           // download_file(sockfd,filename);
        } 
        // else {
    //         std::cout << "No filename provided after RETR." << std::endl;
        
    // }else {
    //     std::cout << "RETR command not found." << std::endl;
    // }
    // }else if(input.find("PASV") != std::string::npos)
    // {
    //     handle_pasv(sockfd);
    }else if(input == "exit")
    {
        e = 1;   
    }else {
        std::cout << "other command" << std::endl;
    }

}
// void send_command(int sock,const std::string &command)
// {
//     send(sock,command.c_str(),command.length(),0);
// }
void send_command(int sock, const std::string &command) {
    std::string cmd_with_newline = command + "\r\n";
    send(sock, cmd_with_newline.c_str(), cmd_with_newline.length(), 0);
}
std::string rec_response(int sock)
{
    char buffer[BUF_SIZE];
    int bytes = recv(sock,buffer,sizeof(buffer),0);
    //std::cout << "Received " << bytes << " bytes." << std::endl; 

    if(bytes > 0)
    {
        buffer[bytes] = '\0';
        return std::string(buffer);
    }
    return "";
}


// void list_files(int sock) {
   
//     send_command(sock, "LIST\r\n");
//     std::string response = rec_response(sock);
//     std::cout << "服务器: " << response << std::endl;
// }
void list_files(int sock) {
    // 发送PASV命令
    send_command(sock, "PASV\r\n");
    std::string pasv_response = rec_response(sock);
    std::cout << "PASV Response: " << pasv_response << std::endl;

    // 判断PASV响应是否包含227
    if (pasv_response.find("227") != std::string::npos) {
        // 解析PASV响应中的IP和端口
        size_t start = pasv_response.find('(');
        size_t end = pasv_response.find(')', start);
        
        if (start != std::string::npos && end != std::string::npos) {
            // 获取IP地址和端口号字符串
            std::string ip_and_port = pasv_response.substr(start + 1, end - start - 1);
            std::cout << "PASV Mode: " << ip_and_port << std::endl;

            // 替换逗号为点，方便解析
            std::replace(ip_and_port.begin(), ip_and_port.end(), ',', '.');

            // 打印格式化的IP
            std::cout << "Formatted IP: " << ip_and_port << std::endl;

            // 提取端口号，PASV的端口号是倒数第二个和最后一个部分
            size_t last_comma = ip_and_port.rfind('.');
            std::string ip = ip_and_port.substr(0, last_comma);
            std::string port_str = ip_and_port.substr(last_comma + 1);
            int port = std::stoi(port_str);
            std::cout << "Port: " << port << std::endl;

            // 建立数据连接
            int data_sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in data_addr;
            data_addr.sin_family = AF_INET;
            data_addr.sin_port = htons(port);

            // 将IP字符串转化为二进制地址
            if (inet_pton(AF_INET, ip.c_str(), &data_addr.sin_addr) <= 0) {
                std::cerr << "Invalid address" << std::endl;
                return;
            }

            // 连接数据端口
            if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) == -1) {
                std::cerr << "Failed to connect to data port." << std::endl;
                return;
            }

            // 发送LIST命令，获取文件列表
            send_command(sock, "LIST\r\n");
            std::string response = rec_response(sock);
            std::cout << "服务器: " << response << std::endl;

            // 从数据连接读取文件列表
            char buffer[BUF_SIZE];
            int bytes;
            while ((bytes = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
                std::cout.write(buffer, bytes);
            }

            // 关闭数据连接
            close(data_sock);
        } else {
            std::cout << "PASV response parsing failed." << std::endl;
        }
    } else {
        std::cout << "PASV mode not established." << std::endl;
    }
}



// void download_file(int sock,const std::string &filename)
// {
//     send_command(sock, "RETR " + filename + "\r\n");
//     std::string response = rec_response(sock);
//       if (response.find("ERROR") != std::string::npos) {
//         std::cout << "File not found." << std::endl;
//         // std::cout << "filename is :" <<filename << std::endl;  
//         return;
//     }

//     std::ofstream file(filename,std::ios::binary);
//     if (!file.is_open()) {
//         std::cout << "Failed to open file for writing." << std::endl;
//         return;
//     }
    
    
//     char buffer[BUF_SIZE];
//     while(1)
//     {
//         int bytes = recv(sock,buffer,sizeof(buffer),0);
//         if (bytes <= 0) {
//             break;
//         }
//         file.write(buffer,bytes);
//         std::cout << "111" << std::endl;
//     }
//     file.close();
//     std::cout << "File downloaded successfully." << std::endl;
// }

// void upload_file(int sock,const std::string &filename)
// {
//    send_command(sock, "STOR " + filename + "\r\n");
//    std::string response = rec_response(sock);
//     if (response.find("ERROR") != std::string::npos) {
//         std::cout << "Cannot upload file." << std::endl;
//         return;
//     }
//     std::ifstream file(filename,std::ios::binary);
//     char buffer[BUF_SIZE];
//     while(file.read(buffer,sizeof(buffer)))
//     {
//         send(sock,buffer,file.gcount(),0);
//     }
//     file.close();
//     std::cout << "File uploaded successfully." << std::endl;

// }

void handle_pasv(int sock) {
    send_command(sock, "PASV\r\n");
    std::string pasv_response = rec_response(sock);
    std::cout << "PASV Response: " << pasv_response << std::endl;

    if (pasv_response.find("227") != std::string::npos) {
        // 提取 IP 地址和端口号
        size_t start = pasv_response.find('(');
        size_t end = pasv_response.find(')', start);
        
        if (start != std::string::npos && end != std::string::npos) {
            std::string ip_and_port = pasv_response.substr(start + 1, end - start - 1);  // 获取 (127,0,0,1,0,21)

            std::cout << "PASV Mode: " << ip_and_port << std::endl;

            // 从字符串中提取 IP 和端口
            std::replace(ip_and_port.begin(), ip_and_port.end(), ',', '.');
            std::cout << "Formatted IP: " << ip_and_port << std::endl;

            // 解析端口号（最后两个数字组成端口）
            size_t last_comma = ip_and_port.rfind('.');
            std::string ip = ip_and_port.substr(0, last_comma);
            std::string port_str = ip_and_port.substr(last_comma + 1);
            int port = std::stoi(port_str);
            std::cout << "Port: " << port << std::endl;
        } else {
            std::cerr << "Failed to parse PASV response." << std::endl;
        }
    } else {
        std::cerr << "PASV mode not established." << std::endl;
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    int list = 0,stor = 0,retr = 0;
    int e = 0;
     std::string command;
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
  

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
     std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::cout << "Connected to the server." << std::endl;
     std::string response = rec_response(sockfd);
    std::cout << "服务器: " << response << std::endl;

    handle_pasv(sockfd);

    while(1)
    {
    std::cout << "enter command: ";
    std::getline(std::cin, command);  

    
    command_parse(command,sockfd,e);

    // if(command == "exit")
    // {
    //     break;
    // }
    if(e == 1)
    {
        break;
        //list_files(sockfd);
        //std::cout << "LIST command" <<std::endl;
    }
    
    // download_file(sockfd,"1.c");
    // upload_file(sockfd,"1.c");
    }
  
    close(sockfd);

    return 0;
}
