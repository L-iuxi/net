    }
    
    
    printf("Server is listening on port %d...\n",RCVPORT);

    remoteadd_len = sizeof(remoteadd);
    while(1)
     {
    if(recvfrom(n, &rbuf, sizeof(rbuf), 0, (struct sockaddr*)&remoteadd, &remoteadd_len) < 0)

    {
        perror("recv error");
        exit(1);
    }
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
