#include <stdint.h>

typedef struct sockaddr SA;


/* IP 소켓 주소 구조: 16바이트 */
struct socket_in {
    uint16_t sin_family; /* 프로토콜 패밀리(항상 32비트 정수: AF_INET): 2바이트 */ 
    uint16_t sin_port; /* 네트워크 바이트 오더로 되어 있는 포트번호: 2바이트 */
    struct in_addr sin_addr; /* 네트워크 바이트 오더로 되어 있는 IP 주소: 4바이트 */
    unsigned char sin_zero[8]; /*  sizeof(struct sockaddr)를 위한 패딩 */
    
};

/* 일반 소켓 주소 구조 (connect, bind, accept를 위함): 16바이트*/
struct socketaddr{
    uint16_t sa_family; /* 프로토콜 패밀리: 2바이트 */
    char sa_data[14]; /* 주소 데이터 */
};