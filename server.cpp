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

        std::stringstream response;

        if(command == "LIST\r\n")
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

                 send_response(client_socket, "150 OK");
                while(!file.eof())
                {
                    file.read(buffer,sizeof(buffer));
                      size_t bytes_read = file.gcount();
                    //send(client_socket,buffer,file.gcount(),0);
                     if (bytes_read > 0)
            {
                // 发送读取的文件内容到客户端
                ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
                
                if (bytes_sent < 0) {
                    // 发送失败，返回错误
                    send_response(client_socket, "550 Error: Transfer failed");
                    file.close();
                    return;
                }
            }
                }
                 send_response(client_socket, "226 Transfer complete");
                    file.close();  
            }else{
                send_response(client_socket, "550 File not found");
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
