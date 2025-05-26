#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <algorithm>  
#include <pwd.h>  
#include <grp.h> 
#include <netdb.h>    
#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024
#define PASV_PORT_MIN 5000
#define PASV_PORT_MAX 6000

using namespace std;

mutex cout_mutex;
atomic<bool> server_running(true);


void log_message(const string& message) {
    lock_guard<mutex> lock(cout_mutex);
    cout << message << endl;
}
int mkdir_p(const string& path) {
    size_t pos = 0;
    string dir;
    int ret = 0;
    
    // 处理绝对路径
    if (path[0] == '/') {
        dir = "/";
        pos = 1;
    }
    
    while ((pos = path.find_first_of('/', pos)) != string::npos) {
        dir = path.substr(0, pos);
        ret = mkdir(dir.c_str(), 0777);
        if (ret != 0 && errno != EEXIST) {
            cerr << "Failed to create directory: " << dir << " - " << strerror(errno) << endl;
            return -1;
        }
        pos++;
    }
    return 0;
}
// 随机端口
int generate_random_port() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(PASV_PORT_MIN, PASV_PORT_MAX);
    return distr(gen);
}


string build_pasv_response(const string& ip, int port) {
    vector<string> ip_parts;
    size_t start = 0;
    size_t end = ip.find('.');
    
    while (end != string::npos) {
        ip_parts.push_back(ip.substr(start, end - start));
        start = end + 1;
        end = ip.find('.', start);
    }
    ip_parts.push_back(ip.substr(start));
    
    int p1 = port / 256;
    int p2 = port % 256;
    
    return "227 Entering Passive Mode (" + ip_parts[0] + "," + ip_parts[1] + "," + 
           ip_parts[2] + "," + ip_parts[3] + "," + to_string(p1) + "," + to_string(p2) + ")\r\n";
}


void handle_data_connection(int data_socket, const string& command, const string& arg) {
    if (command == "LIST") {
        DIR* dir;
        struct dirent* ent;
        string response;
       const char * dir_name = arg.c_str();
       std::cout<<"now dir is:"<<dir_name<<std::endl;
        if ((dir = opendir(dir_name)) != nullptr) {
            while ((ent = readdir(dir)) != nullptr) {
                
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                    continue;
                }
                // response += ent->d_name;
                // response += "\r\n";
                string full_path = string(dir_name) + "/" + ent->d_name;
                struct stat file_stat;
                if (stat(full_path.c_str(), &file_stat) == 0) {
                    response += ((S_ISDIR(file_stat.st_mode))) ? "d" : "-";
                    response += (file_stat.st_mode & S_IRUSR) ? "r" : "-";
                    response += (file_stat.st_mode & S_IWUSR) ? "w" : "-";
                    response += (file_stat.st_mode & S_IXUSR) ? "x" : "-";
                    response += (file_stat.st_mode & S_IRGRP) ? "r" : "-";
                    response += (file_stat.st_mode & S_IWGRP) ? "w" : "-";
                    response += (file_stat.st_mode & S_IXGRP) ? "x" : "-";
                    response += (file_stat.st_mode & S_IROTH) ? "r" : "-";
                    response += (file_stat.st_mode & S_IWOTH) ? "w" : "-";
                    response += (file_stat.st_mode & S_IXOTH) ? "x" : "-";
                
                    response += " " + to_string(file_stat.st_nlink);
                
                    response += " " + to_string(file_stat.st_uid);
                    response += " " + to_string(file_stat.st_gid);
                    
                    response += " " + to_string(file_stat.st_size);
                    
                    char time_buf[80];
                    struct tm* timeinfo = localtime(&file_stat.st_mtime);
                    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", timeinfo);
                    response += " " + string(time_buf);
                    
                    response += " " + string(ent->d_name) + "\r\n";
            }
        }
            closedir(dir);
            send(data_socket, response.c_str(), response.size(), 0);
        }
    } else if (command == "RETR") {
//        struct stat file_stat;
//        string full_path = arg;
//        string filename;
//         if (stat(arg.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
//         string error_msg = "550 File not found or not a regular file\r\n";
//         send(data_socket, error_msg.c_str(), error_msg.size(), 0);
//         return;
//     }
//      size_t last_slash = full_path.find_last_of('/');
//      if (last_slash != string::npos) {
//        filename = full_path.substr(last_slash+1);
//      }
//    // std::cout<<"444  "<<filename<<std::endl;
//         ifstream file(filename, ios::binary);
//         if (file) {
//         char buffer[BUFFER_SIZE];
//         while (file.read(buffer, BUFFER_SIZE)) {
//             if (send(data_socket, buffer, file.gcount(), 0) <= 0) {
//                 break; // 发送失败
//             }
//         }
//         if  (file.gcount() > 0) {
//             std::cout<<"555  "<<buffer<<std::endl;
//             send(data_socket, buffer, file.gcount(), 0);
//              std::cout<<"666  "<<std::endl;
//         }
//         //std::cout<<"777  "<<std::endl;
//         file.close();
 struct stat file_stat;
    if (stat(arg.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
        ifstream file(arg, ios::binary);
        if (file) {
            file.seekg(0, ios::end);
            size_t file_size = file.tellg();
            file.seekg(0, ios::beg);
            
            char* buffer = new char[file_size];
            file.read(buffer, file_size);
            send(data_socket, buffer, file_size, 0);
            std::cout<<"555  "<<buffer<<std::endl;
            delete[] buffer;
            file.close();
            
            const char* ok_msg = "sendokk\r\n";
            send(data_socket, ok_msg, strlen(ok_msg), 0);
    }
}
    
    } else if (command == "STOR") {
        string full_path = arg;
        string filename;
        cout << "Attempting to store file at: " << full_path << endl;
        size_t last_slash = full_path.find_last_of('/');
    if (last_slash != string::npos) {
       filename = full_path.substr(last_slash+1);
        string dir_path = full_path.substr(0, last_slash);
        //  cout << "111" << dir_path << endl;
        if (mkdir_p(dir_path) != 0) {  
            string error_msg = "550 Failed to create directory\r\n";
            send(data_socket, error_msg.c_str(), error_msg.size(), 0);
            return;
        }
    }
        ofstream file(filename, ios::binary);
         std::cout << "222" << filename << std::endl;
        if (file) {
               //std::cout << "222"  << std::endl;
            char buffer[BUFFER_SIZE];
            int bytes_received;
            
            while ((bytes_received = recv(data_socket, buffer, BUFFER_SIZE, 0)) > 0) {
                file.write(buffer, bytes_received);
            }
            file.close();
        }
    }

    
    close(data_socket); 
   
}


void handle_client(int client_socket, const string& server_ip) {
    char buffer[BUFFER_SIZE];
    int data_port = 0;
    int data_socket = -1;
    int pasv_socket = -1;
    sockaddr_in data_addr{};
    socklen_t data_addr_len = sizeof(data_addr);
    
    string welcome_msg = "Welcome ovo\r\n";
    send(client_socket, welcome_msg.c_str(), welcome_msg.size(), 0);
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        string command_str(buffer);
        command_str = command_str.substr(0, command_str.find('\r'));
        log_message("Received: " + command_str);
        
        string command;
        string arg;
        size_t space_pos = command_str.find(' ');
        if (space_pos != string::npos) {
            command = command_str.substr(0, space_pos);
            arg = command_str.substr(space_pos + 1);
        } else {
            command = command_str;
        }
        
        
        ::transform(command.begin(), command.end(), command.begin(), ::toupper);
        
        if (command == "QUIT") {
            string response = "221 Goodbye!\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            break;
        } else if (command == "USER") {
            string response = "enter password ovo\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
       
        }else if (command == "PASV") {
            
            if (pasv_socket != -1) {
                close(pasv_socket);
                pasv_socket = -1;
            }
            
            data_port = generate_random_port();
        
            pasv_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (pasv_socket == -1) {
                string response = "500 Internal server error\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
           
            int opt = 1;
            setsockopt(pasv_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
           
            sockaddr_in pasv_addr{};
            pasv_addr.sin_family = AF_INET;
            pasv_addr.sin_addr.s_addr = INADDR_ANY;
            pasv_addr.sin_port = htons(data_port);
            
            if (bind(pasv_socket, (sockaddr*)&pasv_addr, sizeof(pasv_addr)) == -1) {
                string response = "500 Internal server error\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                close(pasv_socket);
                pasv_socket = -1;
                continue;
            }
            
            if (listen(pasv_socket, 1) == -1) {
                string response = "500 Internal server error\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                close(pasv_socket);
                pasv_socket = -1;
                continue;
            }
            
            string response = build_pasv_response(server_ip, data_port);
            send(client_socket, response.c_str(), response.size(), 0);
        } else if (command == "LIST") {
            if (pasv_socket == -1) {
                string response = "425 Use PORT or PASV first\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
           
            data_socket = accept(pasv_socket, (sockaddr*)&data_addr, &data_addr_len);
            if (data_socket == -1) {
                string response = "425 Can't open data connection\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            string response = "目录打印完毕\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
           
            thread data_thread(handle_data_connection, data_socket, command, arg);
            data_thread.detach();
            
            send(client_socket, response.c_str(), response.size(), 0);
             response = "226 Directory send OK\r\n";
            
           
            close(pasv_socket);
            pasv_socket = -1;
        } else if (command == "RETR") {
            if (pasv_socket == -1) {
                string response = "425 Use PORT or PASV first\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            ifstream file(arg);
            if (!file.good()) {
                string response = "550 File not found\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            file.close();
            
            data_socket = accept(pasv_socket, (sockaddr*)&data_addr, &data_addr_len);
            if (data_socket == -1) {
                string response = "425 Can't open data connection\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            string response = "已成功读取到文件" + arg + "\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            
            thread data_thread(handle_data_connection, data_socket, command, arg);
            data_thread.detach();
            
            response = "下载成功ovo\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            close(pasv_socket);
            pasv_socket = -1;
        } else if (command == "STOR") {
            if (pasv_socket == -1) {
                string response = "425 Use PORT or PASV first\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            data_socket = accept(pasv_socket, (sockaddr*)&data_addr, &data_addr_len);
            if (data_socket == -1) {
                string response = "425 Can't open data connection\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            string response = "150 Ok to send data\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
      
            thread data_thread(handle_data_connection, data_socket, command, arg);
            data_thread.detach();
            
            response = "上传成功\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            
            close(pasv_socket);
            pasv_socket = -1;
        } else {
            string response = "please enter command ovo\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
        }
    }
    
    
    if (pasv_socket != -1) {
        close(pasv_socket);
    }
    close(client_socket);
    
    log_message("Client disconnected");
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }
    
   
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);
    
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Failed to bind to port " << CONTROL_PORT << endl;
        return 1;
    }
    
    if (listen(server_socket, 5) == -1) {
        cerr << "Failed to listen on socket" << endl;
        return 1;
    }
    
 
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent* host = gethostbyname(hostname);
    string server_ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    
    log_message("FTP Server started on " + server_ip + ":" + to_string(CONTROL_PORT));
    log_message("Waiting for connections...");
    
    while (server_running) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        log_message("New connection from " + string(client_ip));

        thread client_thread(handle_client, client_socket, server_ip);
        client_thread.detach();
    }
    
    close(server_socket);
    return 0;
}