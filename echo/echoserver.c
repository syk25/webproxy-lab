#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;  // 클라주소를 위한 구조체
  char client_hostname[MAXLINE],
      client_port[MAXLINE];  // 바이트 단위로 받기 위한 주소, 포트

  // 예외처리
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // 연결 식별자 열기
  listenfd = Open_listenfd(argv[1]);

  //
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connected to (%s %s)\n", client_hostname, client_port);
    echo(connfd);  // 에코함수
    Close(connfd);
  }
  exit(0);
}