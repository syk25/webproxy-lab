#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd; // 클라이언트 식별자 변수
    char *host, *port, buf[MAXLINE]; // 호스트, 포트, 버퍼
    rio_t rio;

    if(argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // 호스트 정보 받기
    port = argv[2]; // 포트 번호 받기

    clientfd = Open_clientfd(host, port); // 
    Rio_readinitb(&rio, clientfd);

    while( Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}