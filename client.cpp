#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>  


#define CONTROL_PORT 21
#define BUF_SIZE 1024


void list_files(int sock);
void upload_file(int sock,const std::string &filename);
void download_file(int sock,const std::string &filename);
void command_parse(const std::string &input,int sockfd)
{

    if (input.find("LIST") != std::string::npos) {
        //std::cout << "Found LIST command!" << std::endl;
        // list = 1;
        list_files(sockfd);
    } else if(input.find("RETR") != std::string::npos)
    {
        //retr = 1;
        std::string command, filename;
        std::istringstream stream(input);  
        stream >> command >> filename;  
        stream >> command >> filename; 
        //std::cout << "filename is :" <<filename << std::endl;  

        if (!filename.empty()) {
            std::cout << "Found RETR command with filename: " << filename << std::endl;
             //upload_file(sockfd,"filename");
            //std::cout << "Found RETR command with filename: " << filename << std::endl;
            // download_file(sockfd,filename);
        } 
        // else {
    //         std::cout << "No filename provided after RETR." << std::endl;

    // }else {
    //     std::cout << "RETR command not found." << std::endl;
    // }
    }else {
        std::cout << "other command" << std::endl;
    }

}
void send_command(int sock,const std::string &command)
{
    send(sock,command.c_str(),command.length(),0);
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

void list_files(int sock)
{
    send_command(sock,"LIST\r\n");
    std::string response;
    while(true)
    {
        std :: string t_response =  rec_response(sock);
    
    if (t_response.empty()) {
            break;  
        }
        response += t_response;
    }
    // = rec_response(sock);
   std::cout << "服务器: " << response << std::endl;
}


// void download_file(int sock,const std::string &filename)
// {
    
//    send_command(sock, "STOR " + filename + "\r\n");
//    std::string response = rec_response(sock);
//     if (response.find("ERROR") != std::string::npos) {
//         std::cout << "Cannot upload file." << std::endl;
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
//     std::ifstream file(filename,std::ios::binary);
    
    
//     char buffer[BUF_SIZE];
//     while(file.read(buffer,sizeof(buffer)))
//     while(1)
//     {
//         send(sock,buffer,file.gcount(),0);
//         int bytes = recv(sock,buffer,sizeof(buffer),0);
//         if (bytes <= 0) {
//             break;
//         }
//         file.write(buffer,bytes);
//         std::cout << "111" << std::endl;
//     }
//     file.close();
//     std::cout << "File uploaded successfully." << std::endl;

//     std::cout << "File downloaded successfully." << std::endl;
// }
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
int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    int list = 0,stor = 0,retr = 0;
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

    std::cout << "pease enter command: ";
    std::getline(std::cin, command);  


    command_parse(command,sockfd);
    if(list == 1)
    {
        list_files(sockfd);
        //std::cout << "LIST command" <<std::endl;
    }

    // download_file(sockfd,"1.c");
    // upload_file(sockfd,"1.c");


    close(sockfd);

    return 0;
}