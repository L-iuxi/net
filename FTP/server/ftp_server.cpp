#include "ftp_server.h"


string FTP::handle_pasv(const string& ip, int port){
    vector<string> ip_depart;
    size_t start = 0;
    size_t end = ip.find('.');

    while(end != std::string::npos)
    {
         ip_depart.push_back(ip.substr(start, end - start));
        start = end + 1;
        end = ip.find('.', start);
    }
    ip_depart.push_back(ip.substr(start));
    
     int p1 = port / 256;
    int p2 = port % 256;
    
    return "227 Entering Passive Mode (" +  ip_depart[0] + "," +  ip_depart[1] + "," +  ip_depart[2] + "," +  ip_depart[3] + "," + to_string(p1) + "," + to_string(p2) + ")\r\n";
}
int FTP::generate_random_port() {
     random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(PASV_PORT_MIN, PASV_PORT_MAX);
    return distr(gen);
}
 
FTP::FTP(int &server_socket) : server_socket(server_socket), threadpool(4) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
        cerr << "Failed to create socket" << endl;
        return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(CONTROL_PORT);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Failed to bind to port " << CONTROL_PORT << endl;
        return;
        }
    
        if (listen(server_socket, 5) == -1) {
        cerr << "Failed to listen on socket" << endl;
        return;
        }
    
        epoll_fd = epoll_create1(0);
        if(epoll_fd == -1)
        {
            cerr<<"epoll创建失败"<<endl;
            return;
        }

        struct epoll_event ep;
        ep.events = EPOLLIN;
        ep.data.fd = server_socket;

         if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ep) == -1) {
        cerr << "将服务器套接字添加到 epoll 失败" << endl;
        return;
    }

    }
void FTP::handle_client(int client_socket,const string& server_ip){
    threadpool.enqueue([this, client_socket, server_ip](){
    char buffer[BUFFER_SIZE];
   // int data_client;
    int pasv_socket = -1;
    int data_port = 0;
    int data_socket = -1;
    sockaddr_in data_addr{};
    socklen_t data_addr_len = sizeof(data_addr);

    string massage = "welcome to my FTP ovo\r\n";
    send(client_socket, massage.c_str(), massage.size(), 0);

    while(true)
    {
        memset(buffer,0,BUFFER_SIZE);
        int bytes_received = recv(client_socket,buffer,BUFFER_SIZE,0);
        if (bytes_received <= 0) {
            cout<<"no bytes received"<<endl;
            break;
        }

        string command_str(buffer);
        command_str = command_str.substr(0, command_str.find('\r'));
        cout<<"server received:"<<command_str<<endl;

        string command;
        string file_name;

        size_t space =command_str.find(' ');
        if(space != std::string::npos)
        {
            command = command_str.substr(0, space);
            file_name = command_str.substr(space + 1);
        }else{
            command = command_str;
        }

        ::transform(command.begin(), command.end(), command.begin(), ::toupper);

        // if(command == "USER")
        // {
        //     cout<<"这个该天再写ovo"<<endl;
        // }else 
        if(command == "PASV")
        {
            if(pasv_socket != -1)
            {
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

            string response = handle_pasv(server_ip,data_port);
            send(client_socket, response.c_str(), response.size(), 0);

        }else if(command == "LIST"){
            if (pasv_socket == -1) {
                string response = "425 Use PORT or PASV first\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            //creat_data(data_socket,pasv_socket,client_socket)
            data_socket = accept(pasv_socket, (sockaddr*)&data_addr, &data_addr_len);
            if (data_socket == -1) {
                string response = "425 Can't open data connection\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            string response = "150\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
          
            handle_list(data_socket,command,file_name);
            response = "226 Directory send OK\r\n";
            send(client_socket, response.c_str(), response.size(), 0);

            close(pasv_socket);
            pasv_socket = -1;
        }else if(command == "RETR"){
            if (pasv_socket == -1) {
                string response = "425 Use PORT or PASV first\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            
            ifstream file(file_name);
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
            
            string response = "已成功读取到文件" + file_name + "\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            handle_retr(data_socket,command, file_name);

            response = "下载成功ovo\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            close(pasv_socket);
            pasv_socket = -1;;
        }else if(command == "STOR"){
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
            
      
          handle_stor(data_socket,command, file_name);
            
            response = "上传成功\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            
            
            close(pasv_socket);
            pasv_socket = -1;
        }else if(command == "PASS")
        {
            cout<<"这个该天再写ovo"<<endl;
        }
    }
    
    if (pasv_socket != -1) {
        close(pasv_socket);
    }
    close(client_socket);
});
}
void FTP::handle_list(int data_socket, const string& command, const string& arg){
    DIR* dir;
    struct dirent* ent;
    string response;
    const char * dir_name = arg.c_str();
   // std::cout<<"now dir is:"<<dir_name<<std::endl;
    
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
         close(data_socket); 
}
void FTP::handle_retr(int data_socket, const string& command, const string& arg){
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
            
            const char* ok_msg = "sendok\r\n";
            send(data_socket, ok_msg, strlen(ok_msg), 0);
    }
}
        close(data_socket);
}
void FTP::handle_stor(int data_socket, const string& command, const string& arg){
     string full_path = arg;
        string filename;
        cout << "Attempting to store file at: " << full_path << endl;
        size_t last_slash = full_path.find_last_of('/');
    if (last_slash != string::npos) {
       filename = full_path.substr(last_slash+1);
        string dir_path = full_path.substr(0, last_slash);
        //  cout << "111" << dir_path << endl;
        // if (mkdir_p(dir_path) != 0) {  
        //     string error_msg = "550 Failed to create directory\r\n";
        //     send(data_socket, error_msg.c_str(), error_msg.size(), 0);
        //     return;
        // }
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
        close(data_socket);
    }

int main()
{
    int server_socket = 0;
    FTP server(server_socket);
    //server.init_ser(server_socket);
    while(true)
    {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }
        
        server.handle_client(client_socket,"127.0.1.1:2100");
    }
    close(server_socket);
    return 0;
}