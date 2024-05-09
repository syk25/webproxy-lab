/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p, *q; // 버퍼, 포인터
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // CLI 입력 인자, 컨텐츠 받기 위한 버퍼 생성
  char method[MAXLINE];
  int n1 = 0, n2 = 0;
  
  /* 인자 2개 추출하기 */
  if((buf = getenv("QUERY_STRING")) != NULL){ // 환경변수의 이름이 존재할때
  p = strchr(buf, '&'); // 포인터에 버퍼 상의 &의 위치를 저장
  *p = '\0'; // &를 Null로 바꿔버림 -> 문자열을 두 부분으로 구분
  strcpy(method, q); // 받은 문자열의 맨 앞을 기준으로 단어 받기
  strcpy(arg1, buf); // 버퍼에서 Null 기준 앞의 문자열을 arg1에 저장
  strcpy(arg2, p+1); // 버퍼에서 Null 기준 뒤의 문자열을 arg2에 저장
  n1 = atoi(arg1); // 문자열을 정수형으로 변환
  n2 = atoi(arg2); // 문자열을 정수형으로 변환
  }

  q = getenv("REQUEST_METHOD");

  /* 응답 body 만들기 */
  sprintf(content, "QUERY_STRING=%s", buf); // 쿼리스트리의 환경변수 저장
  sprintf(content, "Welcome to add.com: "); // content  버퍼에 문자열 저장
  sprintf(content, "%sThe Internet addition portal. \r\n<p>", content); // 현재 내용 뒤에 문자열을 추가 후 저장
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2); // 덧셈 후 결과 출력
  sprintf(content, "%sThanks for visiting!\r\n", content); // 마지막 메세지

  /* HTTP 응답 생성하기 */
  printf("Connection: close\r\n");
  printf("Connection-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("my method: %s\n", q);

  // if((strcmp(q, "HEAD"))){
  //   printf("%s", content);
  // }

  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
