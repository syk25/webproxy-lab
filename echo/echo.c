#include "csapp.h"

void echo(int connfd) {
  size_t n;           // TODO:
  char buf[MAXLINE];  // TODO:
  rio_t rio;          // TODO: 버퍼

  Rio_readinitb(&rio, connfd);  // TODO:
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    printf("server received %d bytes\n", (int)n);
    Rio_writen(connfd, buf, n);
  }
}