
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

#define BUFFER_SIZE 1024

bool ends_with(const char* data, int data_len, const string& suffix) {
    if (data_len < (int)suffix.length()) return false;
    return string(data + data_len - suffix.length(), suffix.length()) == suffix;
}
bool parse_pasv_response(const string& response, string& ip, int& port) {
    size_t start = response.find('(');
    size_t end = response.find(')');
    if (start == string::npos || end == string::npos) {
        return false;
    }
    
    string pasv_data = response.substr(start + 1, end - start - 1);
    vector<string> parts;
    stringstream ss(pasv_data);
    string part;
    
    while (getline(ss, part, ',')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 6) {
        return false;
    }
    
    ip = parts[0] + "." + parts[1] + "." + parts[2] + "." + parts[3];
    port = stoi(parts[4]) * 256 + stoi(parts[5]);
    
    return true;
}


int establish_data_connection(const string& ip, int port) {
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket == -1) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    
    sockaddr_in data_addr{};
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &data_addr.sin_addr);
    
    if (connect(data_socket, (sockaddr*)&data_addr, sizeof(data_addr)) == -1) {
        cerr << "Failed to connect to data port " << port << endl;
        close(data_socket);
        return -1;
    }
    
    return data_socket;
}


string send_ftp_command(int control_socket, const string& command) {
    string full_command = command + "\r\n";
    send(control_socket, full_command.c_str(), full_command.size(), 0);
    
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        return "";
    }
    
    string response(buffer, bytes_received);
    cout << "Server: " << response;
    return response;
}

int main(int argc, char* argv[]) {
    // if (argc < 2) {
    //     cerr << "Usage: " << argv[0] << " <server_ip>" << endl;
    //     return 1;
    // }
    
    //string server_ip = argv[1];
    string server_ip = "127.0.0.1";
    int control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket == -1) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2100);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    if (connect(control_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Failed to connect to server" << endl;
        close(control_socket);
        return 1;
    }
    

    char buffer[BUFFER_SIZE];
    int bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        cerr << "Failed to receive welcome message" << endl;
        close(control_socket);
        return 1;
    }
    
    string welcome_msg(buffer, bytes_received);
    cout << "Server: " << welcome_msg;
    
  
    string username, password;
    cout << "Username: ";
    getline(cin, username);
    send_ftp_command(control_socket, "USER " + username);
    
    cout << "Password: ";
    getline(cin, password);
    send_ftp_command(control_socket, "PASS " + password);
    
    // while (true) {
        cout << "ftp> ";
        string command;
        getline(cin, command);
        
        if (command.empty()) {
            std:: cout << "空命令" << std:: endl;
            //continue;
        }
        
        string cmd = command.substr(0, command.find(' '));
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        
        if (cmd == "QUIT") {
            send_ftp_command(control_socket, "QUIT");
             std:: cout << "退出" << std:: endl;
            //break;
        } else if (cmd == "LIST") {
          // string dirname = command.substr(cmd.length() + 1);
            string dirname;
            if(command.length() > cmd.length())
            {
                dirname = command.substr(cmd.length() + 1);
            }else{
                 std:: cout << "请在LIST 后输入目录名 eg：/home/liuyuxi" << std:: endl;
            }
           string response = send_ftp_command(control_socket, "PASV");
            
            string data_ip;
            int data_port;
            if (!parse_pasv_response(response, data_ip, data_port)) {
                cerr << "Failed to parse PASV response" << endl;
             
            }
            
           
            int data_socket = establish_data_connection(data_ip, data_port);
            if (data_socket == -1) {
                 std:: cout << "failed to establish_data_con" << std:: endl;
            }
            
           
            response = send_ftp_command(control_socket, "LIST "+dirname);
            
            char data_buffer[BUFFER_SIZE];
            while ((bytes_received = recv(data_socket, data_buffer, BUFFER_SIZE, 0)) > 0) {
                cout.write(data_buffer, bytes_received);
            }
            
            close(data_socket);
            
            bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                cout << "Server: " << string(buffer, bytes_received);
            }
        } else if (cmd == "RETR") {
            string filepath = command.substr(cmd.length() + 1);
            
            size_t last_slash = filepath.find_last_of('/');
            string filename = (last_slash == string::npos) ? filepath : filepath.substr(last_slash + 1);
           
            if (filename.empty()) {
        cerr << "输入文件路径 例如/home/liuyuxi/net/FTP/1.txt" << endl;
        //continue;
    }
            string response = send_ftp_command(control_socket, "PASV");
            
            string data_ip;
            int data_port;
            if (!parse_pasv_response(response, data_ip, data_port)) {
                cerr << "Failed to parse PASV response" << endl;
                //continue;
            }
        
            int data_socket = establish_data_connection(data_ip, data_port);
            if (data_socket == -1) {
               // continue;
            }
            
            response = send_ftp_command(control_socket, "RETR " + filepath);
            // if (response.substr(0, 3) != "150") {
            //     close(data_socket);
            //     //continue;
            // }
            
            ofstream file(filename, ios::binary);
            if (!file) {
                cerr << "Failed to create file " << filename << endl;
                close(data_socket);
                //continue;
            }
            
            char data_buffer[BUFFER_SIZE];
           // std::cout<<"test1"<<std::endl;
        
            
        // const string END_MARKER = "sendokk\r\n";
        // bool transfer_complete = false;
       
            while (1) {
                 bytes_received = recv(data_socket, data_buffer, BUFFER_SIZE, 0);
                 if(bytes_received > 0)
                 {
                file.write(data_buffer, bytes_received);

                 }
                 else{
                    std::cout<< "退出" << std::endl;
                    break;
                 }
            }
        
    //         while (!transfer_complete && 
    //    (bytes_received = recv(data_socket, data_buffer, BUFFER_SIZE, 0)) > 0) {
    
    // if (ends_with(data_buffer, bytes_received, END_MARKER)) {
    //     transfer_complete = true;
      
    //     size_t data_len = bytes_received - END_MARKER.length();
    //     if (data_len > 0) {
    //         file.write(data_buffer, data_len);
    //     }
    // } else {
    //     file.write(data_buffer, bytes_received);
    // }
// }
//      if (bytes_received == -1) {
//     perror("recv failed");
// }
            
            file.close();
            close(data_socket);
          

            bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                cout << "Server: " << string(buffer, bytes_received);
            }
        } else if (cmd == "STOR") {
            string filepath = command.substr(cmd.length() + 1);
             ifstream test_file(filepath);
    if (!test_file) {
        cerr << "Error: Local file " << filepath << " does not exist" << endl;
        //continue;
    }
    test_file.close();
            string remote_path = filepath;
            
            ifstream file(filepath, ios::binary);
            if (!file) {
                cerr << " " << filepath << "该路径下文件不存在" << endl;
                //continue;
            }
            file.close();
            
            string response = send_ftp_command(control_socket, "PASV");
            
            string data_ip;
            int data_port;
            if (!parse_pasv_response(response, data_ip, data_port)) {
                cerr << "Failed to parse PASV response" << endl;
                //continue;
            }
            
            int data_socket = establish_data_connection(data_ip, data_port);
            if (data_socket == -1) {
                //continue;
            }
            
            response = send_ftp_command(control_socket, "STOR " + remote_path);
            
            if (response.substr(0, 3) != "150") {
                close(data_socket);
                //continue;
            }
            
           
            file.open(filepath, ios::binary);
            char data_buffer[BUFFER_SIZE];
            while (file.read(data_buffer, BUFFER_SIZE)) {
                send(data_socket, data_buffer, file.gcount(), 0);
            }
            if (file.gcount() > 0) {
                send(data_socket, data_buffer, file.gcount(), 0);
            }
            
            file.close();
            close(data_socket);
            
          
            bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                cout << "Server: " << string(buffer, bytes_received);
            }
        } 
    // }
    
    close(control_socket);
    return 0;
}