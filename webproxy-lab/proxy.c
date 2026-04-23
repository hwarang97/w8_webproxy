#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(int fd, const char *uri, char *hostname, char *port, char *path);

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
  int connfd;
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
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    Close(connfd);
  }
  // printf("%s", user_agent_hdr);
  exit(0);
}

void doit(int fd)
{
  rio_t rio;
  char *buffer[MAXBUF];
  char *method;
  char *uri;
  char *version;
  char *hostname;
  char *port;
  char *path;

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
  if (parse_uri(fd, uri, hostname, port, path) < 0)
  {
    clienterror(fd, uri, "400", "Bad Request", "Proxy could not parse the request URI");
    return;
  }

  // emtpy socket
  read_requesthdrs(&rio);
}

void read_requesthdrs(rio_t *rp)
{
  char buffer[MAXLINE];

  Rio_readlineb(rp, buffer, MAXLINE);
  while ((strcmp(buffer, "\r\n")))
  {
    Rio_readlineb(rp, buffer, MAXLINE);
    printf("%s", buffer);
  }
  return;
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

int parse_uri(int fd, const char *uri, char *hostname, char *port, char *path)
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
