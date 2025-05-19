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
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

//#define PORT 8080
#define CONTROL_PORT 21
#define BUF_SIZE 1024
#define THREAD_POOL_SIZE 4 



std::mutex mtx;  // 保护线程池中的任务队列
std::condition_variable cv;  // 线程池中的任务队列条件变量
std::vector<std::thread> thread_pool;  


std::vector<int> task_queue;

void send_response(int client_socket,const std::string &response)
{
    send(client_socket,response.c_str(),response.length(),0);
    
}

std::string get_ip_and_port(int client_socket) {
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    if (getsockname(client_socket, (struct sockaddr *)&sa, &len) == -1) {
        std::cerr << "Error getting socket information." << std::endl;
        return "";
    }

    // Extract IP and port
    uint32_t ip = ntohl(sa.sin_addr.s_addr);
    uint16_t port = ntohs(sa.sin_port);

    // PASV mode requires the port to be represented as two numbers
    uint8_t p1 = (port >> 8) & 0xFF;
    uint8_t p2 = port & 0xFF;

    std::stringstream ip_and_port;
    ip_and_port << ((ip >> 24) & 0xFF) << ","
                << ((ip >> 16) & 0xFF) << ","
                << ((ip >> 8) & 0xFF) << ","
                << (ip & 0xFF) << ","
                << (int)p1 << ","
                << (int)p2;

    return ip_and_port.str();
}

void handle_client(int client_socket) {
    char buffer[BUF_SIZE];
    bool pasv = false;
    int pasv_socket = -1;

    
    pasv = true; 
    std::string ip_and_port = get_ip_and_port(client_socket);
    if (ip_and_port.empty()) {
        send_response(client_socket, "425 Unable to enter passive mode.\r\n");
        return;
    }
    send_response(client_socket, "227 Entering Passive Mode (" + ip_and_port + ")\r\n");

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes <= 0) {
            break;
        }

        std::string command(buffer);
        std::stringstream response;

        if (command == "LIST\r\n") {
            if (pasv) {
                send_response(client_socket, "150 OK, starting data transfer.\r\n");
                pasv_socket = accept(client_socket, NULL, NULL);
                if (pasv_socket < 0) {
                    send_response(client_socket, "425 Failed to establish data connection.\r\n");
                    return;
                }
            }
            DIR *dir = opendir(".");
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != nullptr) {
                    response << entry->d_name << "\r\n";
                    if (pasv) {
                        send(pasv_socket, response.str().c_str(), response.str().length(), 0);
                    }
                    response.str("");
                }
                closedir(dir);

                if (!pasv) {
                    send_response(client_socket, response.str());
                }
            } else {
                send_response(client_socket, "ERROR1");
            }

            if (pasv) {
                close(pasv_socket);
            }
        }

        else if (command.rfind("RETR ", 0) == 0) {
            std::string filename = command.substr(5);
            if (pasv) {
                send_response(client_socket, "150 Opening data connection.\r\n");

                // Accept the data connection for PASV mode
                pasv_socket = accept(client_socket, NULL, NULL);
                if (pasv_socket < 0) {
                    send_response(client_socket, "425 Failed to establish data connection.\r\n");
                    return;
                }

                std::ifstream file(filename, std::ios::binary);
                if (file.is_open()) {
                    char file_buffer[BUF_SIZE];
                    while (!file.eof()) {
                        file.read(file_buffer, sizeof(file_buffer));
                        ssize_t bytes_read = file.gcount();
                        send(pasv_socket, file_buffer, bytes_read, 0);
                    }
                    file.close();
                    send_response(client_socket, "226 Transfer complete.\r\n");
                } else {
                    send_response(client_socket, "550 File not found.\r\n");
                }
                close(pasv_socket);
            } else {
                send_response(client_socket, "425 No passive connection established.\r\n");
            }
        }
        else if (command.rfind("STOR ", 0) == 0) {
            std::string filename = command.substr(5);
            std::ofstream file(filename, std::ios::binary);
            if (file.is_open()) {
                while (true) {
                    int bytes = recv(client_socket, buffer, sizeof(buffer), 0);

                    if (bytes <= 0) {
                        break;
                    }
                    file.write(buffer, bytes);
                }
                file.close();
                send_response(client_socket, "200 OK\r\n");
            } else {
                send_response(client_socket, "ERROR3: Unable to open file.\r\n");
            }
        } else {
            send_response(client_socket, "ERROR: Unknown command.\r\n");
        }
    }
}


void thread_function()
{
    while (true)
    {
        int client_socket;
        {
            std::unique_lock<std::mutex> lock(mtx);

           
            cv.wait(lock, [] { return !task_queue.empty(); });

           
            client_socket = task_queue.back();
            task_queue.pop_back();
        }

        
        std::cout << "Thread " << std::this_thread::get_id() << " handling client..." << std::endl;
        send_response(client_socket, "220 Welcome to the FTP server\r\n");
        handle_client(client_socket);
        close(client_socket);
        std::cout << "Thread " << std::this_thread::get_id() << " finished handling client." << std::endl;
    }
}
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
     socklen_t client_len = sizeof(client_addr);
    //char buffer[BUF_SIZE];

   
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    server_addr.sin_port =    server_addr.sin_port = htons(CONTROL_PORT);       


    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_fd);
        return 1;
    }

   
    if (listen(server_fd, 3) == -1) {
        perror("Listen failed");
        close(server_fd);
       return 1;
    }
   std::cout << "FTP Server listening on port " << CONTROL_PORT << "..." << std::endl;

   
   for (int i = 0; i < THREAD_POOL_SIZE; ++i)
    {
        thread_pool.emplace_back(thread_function);
    }


    while (true)
    {
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "Client connected." << std::endl;

        {
            std::unique_lock<std::mutex> lock(mtx);
            task_queue.push_back(client_socket);
        }

        
        cv.notify_one();
    }

    close(server_fd);

    return 0;
//    while(1)
//    {
//     int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
//     if(client_socket < 0)
//     {
//         std::cerr << "Failed to accept client connection." << std::endl;
//             continue;
//     }

//      std::cout << "Client connected." << std::endl;
//      send_response(client_socket,"220 Welcome to the FTP server\r\n");
//      handle_client(client_socket);
//      close(client_socket);
//        std::cout << "Client disconnected." << std::endl;

//    }
  
//     close(server_fd);

//     return 0;
}
