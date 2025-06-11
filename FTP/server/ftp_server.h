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
#include <sys/epoll.h>
#include <unistd.h>
#include "thread.h" 

#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024
#define PASV_PORT_MIN 5000
#define PASV_PORT_MAX 6000

using namespace std;
class FTP{
    
    private:
    int server_socket;
    int client_socket;
    int pasv_socket;
    int epoll_fd;

    public:
    
    //void init_ser(int &server_socket);
    void handle_client(int client_socket,const string& server_ip);
    int generate_random_port();
    string handle_pasv(const string& ip, int port);
    FTP(int &server_socket);
    void handle_list(int data_socket, const string& command, const string& arg);
    void handle_retr(int data_socket, const string& command, const string& arg);
    void handle_stor(int data_socket, const string& command, const string& arg);
    Threadpool threadpool;

};