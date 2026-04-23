#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void *thread(void *vargp);
void read_requesthdrs(rio_t *rp, char *host_hdr, size_t host_hdr_size,
                      char *other_hdrs, size_t other_hdrs_size);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(const char *uri, char *hostname, char *port, char *path);
int build_request(char *request, size_t request_size,
                  const char *hostname, const char *port, const char *path,
                  const char *host_hdr, const char *other_hdrs);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  // 리스닝 소켓으부터 연결 소켓 만들기 (루프)

  // connfd 에서 요청줄과 헤더 읽기

  // 클라이언트에게 HTTP 응답 돌려주기

  // 연결 소켓 닫기

  int listenfd;
  int *connfdp;
  pthread_t tid;
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  char client_hostname[MAXLINE];
  char client_port[MAXLINE];

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    /*
     * 각 연결마다 별도의 connfd 저장 공간을 만들고,
     * 그 주소를 새 스레드에 넘겨서 경합 없이 처리한다.
     */
    connfdp = Malloc(sizeof(int));
    clientlen = sizeof(struct sockaddr_storage);
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    /*
     * 메인 스레드는 일을 worker thread에게 넘긴 뒤
     * 바로 다음 accept로 돌아가서 새 연결을 받을 수 있다.
     */
    Pthread_create(&tid, NULL, thread, connfdp);
  }
  // printf("%s", user_agent_hdr);
  exit(0);
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);

  /*
   * 이 스레드는 작업이 끝나면 join 없이 자동으로 정리되도록 detached로 둔다.
   */
  Pthread_detach(Pthread_self());
  Free(vargp);

  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int fd)
{
  rio_t rio;
  rio_t server_rio;
  char buffer[MAXLINE];
  char method[MAXLINE];
  char uri[MAXLINE];
  char version[MAXLINE];
  char hostname[MAXLINE];
  char port[MAXLINE];
  char path[MAXLINE];
  char host_hdr[MAXLINE];
  char other_hdrs[MAXBUF];
  char request[MAXBUF];
  char response_buf[MAXBUF];
  int serverfd;
  ssize_t n;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buffer, MAXLINE);

  // extract method, uri, version
  sscanf(buffer, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  // extract hostname, port, path
  if (parse_uri(uri, hostname, port, path) < 0)
  {
    clienterror(fd, uri, "400", "Bad Request", "Proxy could not parse the request URI");
    return;
  }

  // 브라우저가 보낸 나머지 헤더들을 읽어서 분류한다.
  // Host는 따로 저장하고, 나머지 전달할 헤더는 other_hdrs에 모아둔다.
  read_requesthdrs(&rio, host_hdr, sizeof(host_hdr), other_hdrs, sizeof(other_hdrs));

  /*
   * tiny 서버로 보낼 "완성된 HTTP 요청 문자열"을 만든다.
   * request line + Host + User-Agent + Connection 계열 + 기타 헤더 순서로 합친다.
   */
  if (build_request(request, sizeof(request),
                    hostname, port, path, host_hdr, other_hdrs) < 0)
  {
    clienterror(fd, uri, "500", "Internal Server Error",
                "Proxy could not build the request");
    return;
  }

  /*
   * 이제 tiny 서버와 실제 TCP 연결을 맺는다.
   * 여기서 실패하면 프록시는 브라우저에게 에러 응답을 돌려준다.
   */
  serverfd = open_clientfd(hostname, port);
  if (serverfd < 0)
  {
    clienterror(fd, hostname, "502", "Bad Gateway",
                "Proxy could not connect to the end server");
    return;
  }

  /*
   * 위에서 만든 HTTP 요청 문자열을 tiny 서버로 전송한다.
   * 이 순간부터 프록시는 "클라이언트 대신 tiny에게 요청을 보내는 역할"을 하게 된다.
   */
  Rio_writen(serverfd, request, strlen(request));

  /*
   * tiny 서버 응답은 텍스트일 수도 있고 이미지 같은 바이너리일 수도 있다.
   * 따라서 줄 단위가 아니라 "바이트 그대로" 읽어서 클라이언트로 전달한다.
   */
  Rio_readinitb(&server_rio, serverfd);
  while ((n = Rio_readnb(&server_rio, response_buf, sizeof(response_buf))) > 0)
  {
    Rio_writen(fd, response_buf, n);
  }

  /* tiny 서버와의 연결은 이번 요청이 끝났으므로 닫는다. */
  Close(serverfd);
}

void read_requesthdrs(rio_t *rp, char *host_hdr, size_t host_hdr_size,
                      char *other_hdrs, size_t other_hdrs_size)
{
  char buffer[MAXLINE];

  /* 헤더가 하나도 없는 경우를 대비해 출력 버퍼를 먼저 비운다. */
  host_hdr[0] = '\0';
  other_hdrs[0] = '\0';

  while (Rio_readlineb(rp, buffer, MAXLINE) > 0)
  {
    /* 빈 줄을 만나면 요청 헤더가 끝난 것이다. */
    if (!strcmp(buffer, "\r\n"))
    {
      break;
    }

    /* Host 헤더는 따로 저장해 두었다가 나중에 그대로 재사용한다. */
    if (!strncasecmp(buffer, "Host:", 5))
    {
      snprintf(host_hdr, host_hdr_size, "%s", buffer);
      continue;
    }

    /*
     * 아래 세 헤더는 프록시가 직접 다시 만들어서 보낼 것이므로
     * 브라우저가 보낸 값은 버린다.
     */
    if (!strncasecmp(buffer, "Connection:", 11) ||
        !strncasecmp(buffer, "Proxy-Connection:", 17) ||
        !strncasecmp(buffer, "User-Agent:", 11))
    {
      continue;
    }

    /*
     * 그 외의 헤더는 tiny 서버로 그대로 전달할 수 있도록
     * other_hdrs 버퍼 뒤에 차곡차곡 붙여 둔다.
     */
    if (strlen(other_hdrs) + strlen(buffer) < other_hdrs_size)
    {
      strncat(other_hdrs, buffer,
              other_hdrs_size - strlen(other_hdrs) - 1);
    }
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char body[MAXLINE];
  char buffer[MAXLINE];

  // http response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // print http response (header + body)
  sprintf(buffer, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buffer, strlen(buffer));
  sprintf(buffer, "Content-type: text/html\r\n");
  Rio_writen(fd, buffer, strlen(buffer));
  sprintf(buffer, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buffer, strlen(buffer));
  Rio_writen(fd, body, strlen(body));
}

int parse_uri(const char *uri, char *hostname, char *port, char *path)
{
  char buffer[MAXLINE];
  char *domain = buffer;
  char *path_begin = NULL;
  char *colon = NULL;
  char *port_ptr = NULL;

  strcpy(buffer, uri);

  if (strncmp(buffer, "http://", 7) != 0)
  {
    return -1;
  }
  domain = domain + 7;

  // get path
  path_begin = strchr(domain, '/');
  if (path_begin == NULL)
  {
    strcpy(path, "/");
  }
  else
  {
    strcpy(path, path_begin);
    *path_begin = '\0';
  }

  // get port
  colon = strchr(domain, ':');
  if ((colon == NULL))
  {
    strcpy(port, "80");
  }
  else
  {
    // there is :, but no port number
    if (*(colon + 1) == '\0')
    {
      return -1;
    }

    // valid case
    else
    {
      *colon = '\0';
      colon += 1;
      strcpy(port, colon);
    }
  }

  // port is consisted of number
  port_ptr = port;
  while (*port_ptr != '\0')
  {
    if (!isdigit((unsigned char)*port_ptr))
    {
      return -1;
    }
    port_ptr++;
  }

  // get hostname
  if (*domain == '\0')
  {
    return -1;
  }
  else
  {
    strcpy(hostname, domain);
  }
  return 0;
}

int build_request(char *request, size_t request_size,
                  const char *hostname, const char *port, const char *path,
                  const char *host_hdr, const char *other_hdrs)
{
  char generated_host_hdr[MAXLINE];
  const char *final_host_hdr;
  int written;

  /*
   * 브라우저가 Host 헤더를 보냈다면 그 값을 그대로 사용한다.
   * 없었다면 parse_uri 결과를 바탕으로 Host 헤더를 직접 만든다.
   */
  if (host_hdr[0] != '\0')
  {
    final_host_hdr = host_hdr;
  }
  else
  {
    if (!strcmp(port, "80"))
    {
      snprintf(generated_host_hdr, sizeof(generated_host_hdr),
               "Host: %s\r\n", hostname);
    }
    else
    {
      snprintf(generated_host_hdr, sizeof(generated_host_hdr),
               "Host: %s:%s\r\n", hostname, port);
    }
    final_host_hdr = generated_host_hdr;
  }

  /*
   * 프록시가 tiny 서버로 보낼 최종 요청 형식:
   * 1. GET /path HTTP/1.0
   * 2. Host
   * 3. User-Agent
   * 4. Connection: close
   * 5. Proxy-Connection: close
   * 6. 브라우저가 보낸 기타 헤더
   * 7. 마지막 빈 줄
   */
  written = snprintf(request, request_size,
                     "GET %s HTTP/1.0\r\n"
                     "%s"
                     "%s"
                     "Connection: close\r\n"
                     "Proxy-Connection: close\r\n"
                     "%s"
                     "\r\n",
                     path,
                     final_host_hdr,
                     user_agent_hdr,
                     other_hdrs);

  if (written < 0 || (size_t)written >= request_size)
  {
    return -1;
  }

  return 0;
}
