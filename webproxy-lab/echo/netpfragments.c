#include "csapp.h"

void fragments()
{
    int clientfd;

    /* $begin socketcall */
    clientfd = Socket(AF_INET, SOCK_STREAM, 0);
    /* $end socketcall */

    clientfd = clientfd; /* keep gcc happy */

    /* $begin inaddr */
    /* IP address structure */
    struct in_addr
    {
        uint32_t s_addr; /* Address in network byte order (big-endian) */
    };
    /* $end inaddr */

    /* $begin addrinfo */
    struct addrinfo
    {
        int ai_flags;             /* Hints argument flags */
        int ai_family;            /* First arg to socket function */
        int ai_socktype;          /* Second arg to socket function */
        int ai_protocol;          /* Third arg to socket function  */
        char *ai_canonname;       /* Canonical hostname */
        size_t ai_addrlen;        /* Size of ai_addr struct */
        struct sockaddr *ai_addr; /* Ptr to socket address structure */
        struct addrinfo *ai_next; /* Ptr to next item in linked list */
    };
    /* $end addrinfo */

    /* $begin sockaddr */
    /* IP socket address structure */
    struct sockaddr_in
    {
        uint16_t sin_family;       /* Protocol family (always AF_INET) */
        uint16_t sin_port;         /* Port number in network byte order */
        struct in_addr sin_addr;   /* IP address in network byte order */
        unsigned char sin_zero[8]; /* Pad to sizeof(struct sockaddr) */
    };

    /* Generic socket address structure (for connect, bind, and accept) */
    struct sockaddr
    {
        uint16_t sa_family; /* Protocol family */
        char sa_data[14];   /* Address data  */
    };
    /* $end sockaddr */
}
#include "csapp.h"

무효 조각()
{
    int 클라이언트fd;

    /* $begin 소켓콜 */
    clientfd = 소켓(AF_INET, SOCK_STREAM, 0);
    /* $end 소켓콜 */

    클라이언트fd = 클라이언트fd; /* gcc를 행복하게 유지 */

    /* $begin inaddr */
    /* IP 주소 구조 */
    구조체 in_addr
    {
        uint32_t s_addr; /* 네트워크 바이트 순서의 주소(빅엔디안) */
    };
    /* $end inaddr */

    /* $begin addrinfo */
    구조체 추가 정보
    {
        int ai_flags;             /* 인수 플래그 힌트 */
        int ai_family;            /* 소켓 함수에 대한 첫 번째 인수 */
        int ai_socktype;          /* 소켓 함수에 대한 두 번째 인수 */
        int ai_protocol;          /* 소켓 함수에 대한 세 번째 인수 */
        char *ai_canonname;       /* 정식 호스트 이름 */
        size_t ai_addrlen;        /* ai_addr 구조체의 크기 */
        구조체 sockaddr *ai_addr; /* 소켓 주소 구조에 대한 Ptr */
        구조체 addrinfo *ai_next; /* 연결리스트의 다음 항목으로 Ptr */
    };
    /* $end addrinfo */

    /* $begin sockaddr */
    /* IP 소켓 주소 구조 */
    구조체 sockaddr_in
    {
        uint16_t sin_family;        /* 프로토콜 계열(항상 AF_INET) */
        uint16_t sin_port;          /* 네트워크 바이트 순서의 포트 번호 */
        구조체 in_addr sin_addr;    /* 네트워크 바이트 순서의 IP 주소 */
        부호 없는 char sin_zero[8]; /* sizeof(struct sockaddr)에 맞게 패드 */
    };

    /* 일반 소켓 주소 구조(연결, 바인딩 및 수락용) */
    구조체 sockaddr
    {
        uint16_t sa_family; /* 프로토콜 계열 */
        char sa_data[14];   /* 주소 데이터 */
    };
    /* $end sockaddr */
}
