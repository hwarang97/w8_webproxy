/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd)
{
  // 소켓을 읽어들이고, static인지 dynamic인지를 판단한 뒤에, serve_static() 또는 serve_dynamic()을 호출한다.

  rio_t rio;
  int is_static;
  struct stat sbuffer;
  char buffer[MAXLINE];
  char method[16]; // any method length is not longher than 6.
  char uri[MAXLINE];
  char version[16];
  char filename[MAXLINE];
  char cgiargs[MAXLINE];

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buffer, MAXLINE);
  sscanf(buffer, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    //  clienterror();  // TODO: implement clienterror function which flush error message with socket
    return;
  }

  // empty rio internal buffer, it's not essential but conventinal
  // we don't need to reuse connfd, so empty rio buffer later than GET if statement
  // TODO: implement read_requesthdrs
  read_requesthdrs(&rio);

  // parse uri to detemine whether static request, filename, cgiargs
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuffer) < 0)
  {
    // clienterror(); // TODO: implement clienterror function which flush error message with socket
    return;
  }

  // branch static, dynamic
  if (is_static)
  {
    if (!(S_ISREG(sbuffer.st_mode)) || !(S_IRUSR & sbuffer.st_mode))
    {
      // clienterror(); // TODO: implement clienterror function which flush error message with socket
      return;
    }
    serve_static(fd, filename, sbuffer.st_size); // TODO: implement serve_static
  }
  else
  {
    if (!(S_ISREG(sbuffer.st_mode)) || !(S_IXUSR & sbuffer.st_mode))
    {
      // clienterror(); // TODO: implement clienterror function which flush error message with socket
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // TODO: implement serve_dynamic
  }
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

void get_filetype(char *filename, char *filetype)
{
  // return file MIME type
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }

  return;
}

// TODO: implement
// int parse_uri(char *uri, char *filename, char *cgiargs);
// void serve_static(int fd, char *filename, int filesize);
// void serve_dynamic(int fd, char *filename, char *cgiargs);
