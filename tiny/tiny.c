/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */

/* tiny 서버: 정적 컨텐츠와 동적 컨텐츠를 GET 메서드를 통해 제공하는 서버 */
#include "csapp.h"

///////////////////////////////////////////////////////////////////////////////
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  // 커맨드 라인 오류인 경우 종료
  if (argc != 2) { // 인자의 갯수가 맞지 않은 경우: 포트번호를 입력하지 않은 경우
    fprintf(stderr, "usage: %s <port>\n", argv[0]); //  stderr 스트림이 흐를 때 frpintf는 해당 문자열을 출력
    exit(1);
  }


  listenfd = Open_listenfd(argv[1]); // 리스닝 소켓 열기 -> 식별자 받기
  while (1) {
    clientlen = sizeof(clientaddr); // 클라이언트의 소켓주소의 크기 저장
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}


// TODO:
// doit(): HTTP 트랜젝션 담당
/**
 * doit()함수
 * 기능: 하나의 HTTP 트랜젝션 담당
 * 특징
 * 1) HTTP GET 메서드만 지원
 * 2) 다른 요청이 들어올 때 에러 메세지 반환 후 본래의 루틴으로 돌아옴
 * 
 * 동작과정
 * 1) requestline 읽고 파싱하기:rio_readlineb
 * 2) GET 요청이 아닌 경우 예외처리 및 함수 반환(이후 메인에서 연결 종료함)
 * 3) 
 * 4) GET 요청인 경우, 컨텐츠의 종류에 따라 요청 처리하기
 * 
 */

void doit(int fd) {  // 식별자를 인자로 받음
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* 헤더와 요청 줄 읽기 */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers: \n");
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET")) {  // GET 메세지가 아닌 경우 에러 메세지 출력
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  /* GET 요청으로부터 URI 파싱하기 */
  // FIX
  is_static = parse_uri(uri, filename, cgiargs);
  printf("File name: %s\n", filename);
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  /* 정적 컨텐츠 보내기 */
  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  } else { /* 동적 컨텐츠 보내기 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

///////////////////////////////////////////////////////////////////////////////
// TODO:
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
///////////////////////////////////////////////////////////////////////////////
// TODO:
/* parse_uri함수
 * tiny는 정적컨첸츠와 동적 컨텐츠에 대해 다음과 같은 전제를 한다.
 * ./cgi_bin: 실행가능한 파일들(동적 컨텐츠)가 모여 있는 경로
 * 현재경로: 정적 컨텐츠 파일들이 모여 있는 경로
 */

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  /* 정적 컨텐츠 */
  if (!strstr(uri, "cgi-bin")) {  // 동적 컨첸츠 경로가 아닌 경우
    strcpy(cgiargs, "");          // cgi 관련 인자 문자열 비우기
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  }
  /* 동적 컨텐츠 */
  else {
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}
///////////////////////////////////////////////////////////////////////////////
// TODO:
/* serve_static:  */

void serve_static(int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* 클라이언트에게 응답 헤더 보내기 */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* 클라이언트에게 응답 바디 보내기 */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}
///////////////////////////////////////////////////////////////////////////////
// TODO:
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  } else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  } else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  } else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  } else {
    strcpy(filetype, "text/plain");
  }
}
///////////////////////////////////////////////////////////////////////////////
// TODO:
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* HTTP 응답의 첫번째 부분 반환 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // Child
  if (Fork() == 0) {
    /* Real Server는 모든 CGI 변수를 여기로 둠 (???)*/
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
///////////////////////////////////////////////////////////////////////////////
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {  // TODO:
  char buf[MAXLINE], body[MAXBUF];

  /* HTTP 응답 body 만들기 */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body,
          "%s<body bgcolor="
          "ffffff"
          ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  /* HTTP 응답 출력하기 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}