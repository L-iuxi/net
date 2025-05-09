#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include<string.h>
#include <unistd.h>


#define NAMESIZE  11
#define IPSTRLEN  40
#define RCVPORT   12456
struct msg_st{
    char name[NAMESIZE];
    int math;
    int chinese;
}__attribute__((packed));

int main(int argc,char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <IP address>\n", argv[0]);
        exit(1);
    }

    int n;
    // struct msg_st sendbuf;
    // struct socketadd_in remoteadd;
    struct msg_st sendbuf;
    struct sockaddr_in remoteadd;
    n = socket(AF_INET,SOCK_DGRAM,0);
    if(n < 0)
    {
        perror("socket");
        exit(1);
    }
    
    strcpy(sendbuf.name,"liux");
    sendbuf.math = 100;
    sendbuf.chinese = 100;

   // sendbuf.math = htonl(rand()%100);

    remoteadd.sin_family = AF_INET;
    //remoteadd.sin_port = htons(atoi(RCVPORT));
    remoteadd.sin_port = htons(RCVPORT); 

   // remoteadd.sin_addr = htons(atoi(argv[1]));
    //inet_pton(AF_INET,argv[1],&remoteadd.sin_addr);
    //inet_pton(AF_INET, argv[1], &remoteadd.sin_addr);
    //inet_pton(AF_INET, argv[1], &remoteadd.sin_addr);  
    if (inet_pton(AF_INET, argv[1], &remoteadd.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    // if(sendto(n,sendbuf,sizeof(sendbuf),(void*)remoteadd,sizeof(remoteadd))<0);
    if (sendto(n, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr*)&remoteadd, sizeof(remoteadd)) < 0) {
        perror("sendto error");
        exit(1);
    }
    puts("OK");
   
    close(n);
}