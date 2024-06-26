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

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *version, int check);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *version, int check, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  // 커맨드 라인 오류인 경우 종료
  if (argc !=
      2) {  // 인자의 갯수가 맞지 않은 경우: 포트번호를 입력하지 않은 경우
    fprintf(stderr, "usage: %s <port>\n",
            argv[0]);  //  stderr 스트림이 흐를 때 frpintf는 해당 문자열을 출력
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);  // 리스닝 소켓 열기 -> 식별자 받기
  while (1) {
    clientlen = sizeof(clientaddr);  // 클라이언트의 소켓주소의 크기 저장
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

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
  int is_static, check = -1;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* 헤더와 요청 줄 읽기 */
  Rio_readinitb(&rio, fd);
  if (!Rio_readlineb(&rio, buf, MAXLINE)) {
    return;
  }

  printf("Request headers: \n");
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  // GET 메세지가 아닌 경우 에러 메세지 출력
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }

  // 메서드를 찾기 위한 초기화
  if(!strcasecmp(method, "GET")){ // GET 메서드인 경우
    check = 0;
  } else if(!strcasecmp(method, "HEAD")){ // HEAD 메서드인 경우
    check = 1;
  }

  read_requesthdrs(&rio);  // 요청 헤더 읽기

  /* GET 요청으로부터 URI 파싱하기 */
  is_static = parse_uri(uri, filename,
                        cgiargs);  // URI를 파일이름으로 파싱함, CGI 인자 비움,
                                   // 요청이 정적인지, 동적인지 확인
  printf("File name: %s\n", filename);
  if (stat(filename, &sbuf) <
      0) {  // 파일이 존재하지 않는 경우, 에러 메세지 반환
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  /* 정적 컨텐츠 보내기 */
  if (is_static) {  // 파일이 정적 컨텐츠인 경우
    if (!(S_ISREG(sbuf.st_mode)) ||
        !(S_IRUSR & sbuf.st_mode)) {  // 1) 정규 파일이 아니면 2) 사용자가 읽을
                                      // 권한이 없으면
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");  // 에러 메세지 출력
      return;
    }
    serve_static(fd, filename, sbuf.st_size, version, check);  // 정적 컨텐츠 전달
  } else { /* 동적 컨텐츠 보내기 */
    if (!(S_ISREG(sbuf.st_mode)) ||
        !(S_IXUSR & sbuf.st_mode)) {  // 1) 정규 파일이 아니면 2) 사용자가 읽을
                                      // 권한이 없다면
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");  // 에러 메세지 출력
      return;
    }
    serve_dynamic(fd, filename, cgiargs, version, check, method);  // 동적 컨텐츠 전달
  }
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

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

/* serve_static:  */

void serve_static(int fd, char *filename, int filesize, char *version, int check) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* 클라이언트에게 응답 헤더 보내기 */
  get_filetype(filename, filetype);
  sprintf(buf, "%s 200 OK\r\n", version);
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* 클라이언트에게 응답 바디 보내기 */

  /**
   * 숙제: 11.9
   * 1) mmap 대체하기
   * 2) rio_writen 대체하기
   *
   * mmap 함수
   * 선언: void *mmap(void *start, size_t length, int prot, int flags, int fd,
   * off_t offset); 소개: 메모리의 특정공간에 파일을 매핑 -> 성능개선! 입력:
   *  - start: 매핑할 메모리의 시작 주소
   *  - length: 매핑할 메모리의 길이
   *  - prot: 사용권한
   *  - flags: 메모리 주소 공간 설정
   *  - fd: 파일 식별자
   *  - offset: 시작주소에서 떨어진 거리
   * 출력: 없음
   * 동작과정
   *  1. mmap 실행 후 가상 메모리 주소에 file 주소가 매핑 됨
   *  2. 해당 메모리에 접근할 때 OS가 파일 데이터를 복사하여 물리 메모리에
   * 업로드
   *  3. 메모리 read
   *
   * rio-writen
   * 선언: ssize_t rio_writen(int fd, void *usrbuf, size_t n);
   * 입력:
   *  - fd: 파일 식별자
   *  - usrbuf: 쓸 데이터를 가지고 있는 버퍼의 포인터
   *  - n: 쓰려는 바이트의 수
   * 출력:
   * 동작과정: 네트워크 프로그래밍에서 안정적으로 데이터를 쓰기 위한 함수(요청한
   * 바이트 수 만큼 씀)
   *
   * malloc
   * 선언: void *malloc(size_t size);
   * 입력: size: 할당 받을 바이트의 수
   * 동작과정:
   *  - 요청 처리
   *  - 메모리 검색: 힙에서 요청 된 데이터 검색
   *  - 메모리 할당: 적절한 블럭을 찾은 후 사용자에게 할당
   *  - 메모리 분할: 블럭의 크기가 요청 크기보다 크다면 남은 공간 분할
   *  - 메모리 초기화 안함
   *  - 에러 처리: 요청을 처리할 수 없는 경우 NULL 포인터 반환
   *
   * rio_readn
   * 선언:
   * 입력:
   * 출력:
   * 동작과정:
   *
   * rio_writen
   * 입력:
   * 출력:
   * 동작과정:
   *  */

  // 바꿔야할 코드
  // srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd);
  // Rio_writen(fd, srcp, filesize);
  // Munmap(srcp, filesize);

  // 바꾼코드
  if(check == 0){
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = (char *)Malloc(filesize);  // 파일 크기 만큼 동적할당 후 주소 반환
  Rio_readn(srcfd, srcp, filesize);  // 파일을 읽고 srcp 에 저장
  Close(srcfd);                      // 연결 끊기
  Rio_writen(fd, srcp, filesize);  // 클라이언트에게 파일을 보낸다
  Free(srcp);                      // 메모리 해제
  }else{
    return;
  }
  
}

void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  } else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  } else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  } else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  } else if (strstr(filename, ".mp4")) {
    strcpy(filetype, "video/mp4");  // mp4 파일 처리
  } else {
    strcpy(filetype, "text/plain");
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *version, int check, char *method) {
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* HTTP 응답의 첫번째 부분 반환 */
  sprintf(buf, "%s 200 OK\r\n", version);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // Child
  if (Fork() == 0) {
    /* Real Server는 모든 CGI 변수를 여기로 둠*/
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    
    printf("%s\n", method);
    
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
/**
 * clienterror()
 *
 */

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE];  // HTTP 응답헤더 저장할 버퍼
  char body[MAXBUF];  // HTTP 응답본문 저장할 버퍼

  /* HTTP 응답 body 만들기 */
  sprintf(body, "<html><title>Tiny Error</title>");  // html 파일 제목
  sprintf(body,
          "%s<body bgcolor="
          "ffffff"
          ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum,
          shortmsg);  // 버퍼에 형식 문자열을 대체해서 저장
  sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);          // 상동
  sprintf(body, "%s<hr><em>The Tiny Web Server</em></hr>\r\n", body);  // 상동

  /* HTTP 응답 헤더 만들기 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum,
          shortmsg);  // 상동: buf에  HTTP/1.0 에러코드 에러메세지 쓰기
  Rio_writen(
      fd, buf,
      strlen(buf));  // 버퍼에서 strlen(buf)의 길이만큼 꺼내서 식별자가 가리키는
                     // 파일(fd)에 씀(참고로 버퍼에는 어떤 영향도 가지 않음)
  sprintf(buf, "Content-type: text/html\r\n");  // 버퍼에 데이터 씀
  Rio_writen(fd, buf, strlen(buf));  // fd에 버퍼의 길이만큼 씀
  sprintf(buf, "content-length: %d\r\n\r\n", (int)strlen(body));  // 상동
  Rio_writen(fd, buf, strlen(buf));    // 버퍼 메세지 쓰기
  Rio_writen(fd, body, strlen(body));  // HTTP 바디 쓰기
}