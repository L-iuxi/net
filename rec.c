#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>


#define NAMESIZE  11
#define IPSTRLEN  40
#define RCVPORT "12345"
struct msg_st{
    uint8_t name[NAMESIZE];
    int math;
    int chinese;
}__attribute__((packed));

int main()
{
    int n;
    struct sockaddr_in localadd,remoteadd;
    struct msg_st rbuf;
    socklen_t remoteadd_len;
    char ipstr[IPSTRLEN];

    n = socket(AF_INET,SOCK_DGRAM,0);
    if(n < 0)
    {
        perror("socket");
        exit(1);
    }

    localadd.sin_family = AF_INET;
    //inet_pton localadd.sin_port = htons(atoi(RCVPORT));
    inet_pton(AF_INET, "0.0.0.0", &localadd.sin_addr);;

    int m = bind(n,(void*)&localadd,sizeof(localadd));
    if(m < 0)
    {
        perror("");

    }

    remoteadd_len = sizeof(remoteadd);
    while(1)
    {
    recvfrom(n,&rbuf,sizeof(rbuf),0,(void*)&localadd,&remoteadd_len);
    inet_ntop(AF_INET,&remoteadd.sin_addr,ipstr,IPSTRLEN);  
    // printf("massage from %s %n\n",remoteadd.sin_addr,ntohs(remoteadd.sin_port));
    printf("message from %s %d\n", ipstr, ntohs(remoteadd.sin_port));

    printf("Name:%s\n",rbuf.name);
    printf("math:%d\n",ntohl(rbuf.math));
    printf("chinese:%d\n",ntohl(rbuf.chinese));
    }
    close(n);


    return 0;
}
