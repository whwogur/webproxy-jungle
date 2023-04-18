#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr ="User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection : close\r\n";
static const char *prox_hdr = "Proxy-Connection : close\r\n";
static const char *host_hdr_format = "Host : %s\r\n";
static const char *requestLint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";

static const char *connection_key = "Connection";
static const char *user_agent_key = "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

void doit(int);
void parse_uri(char*,char*,char*,int*);
void build_http_header(char*, char*, char*, int, rio_t*);
int connect_endServer(char*, int, char*);

/*
 * main : 프록시 서버에 사용할 포트 번호를 인자로 받는다
 *        프록시 서버가 클라이언트와 연결할 연결 소켓 connfd 만들고 doit()
*/
int main(int argc,char **argv)
{
    int listenfd,connfd;
    socklen_t  clientlen;
    char hostname[MAXLINE],port[MAXLINE];

    struct sockaddr_storage clientaddr;/*generic sockaddr struct which is 28 Bytes.The same use as sockaddr*/

    if(argc != 2){
        fprintf(stderr,"usage :%s <port> \n",argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);

        /*print accepted message*/
        Getnameinfo((SA*)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
        printf("Accepted connection from (%s %s).\n",hostname,port);

        /*sequential handle the client transaction*/
        doit(connfd);

        Close(connfd);
    }
    return 0;
}
/*
 * doit() : 1) 클라이언트의 요청 라인을 파싱해 엔드 서버의 hostname, path, port를 가져오고
 *          2) 엔드 서버에 보낼 요청 라인과 헤더를 만들 변수들을 만든다.
 *          3) 프록시 서버와 엔드 서버를 연결하고 엔드 서버의 응답 메시지를 클라이언트에게 준다.
*/
/* handle the client HTTP transaction */
void doit(int connfd)
{
  int end_serverfd; // 엔드 서버 디스크립터

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char endserver_http_header[MAXLINE]; // 엔드 서버에 보낼 프록시 서버의 요청 라인과 헤더
  /* 요청 내용 저장 */
  char hostname[MAXLINE], path[MAXLINE]; // 프록시 서버가 파싱한 엔드 서버의 정보(클라이언트가 요청함)
  int port;

  rio_t rio, server_rio; // rio - 클라이언트 rio / server_rio - 엔드서버 rio

  /* 클라이언트가 보낸 요청 헤더에서 method, uri, version 가져옴 */
  /* GET http://localhost:7777/home.html HTTP/1.1 */
  Rio_readinitb(&rio, connfd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version); // 클라이언트 요청 읽어옴

  if(strcasecmp(method, "GET")) {
    printf("Proxy does not implement the method");
    return;
  }

  /* uri 파싱 -> hostname, path, port */
  /* hostname -> localhost, path -> /home.html, port -> 7777 */
  parse_uri(uri, hostname, path, &port);

  // 프록시 서버가 엔드 서버로 보낼 요청 헤더들을 만듦. endserver_http_header가 채워진다.
  build_http_header(endserver_http_header, hostname, path, port, &rio);

  /* 프록시와 엔드 서버 연결 */
  end_serverfd = connect_endServer(hostname, port, endserver_http_header);
  // clientfd connected from proxy to end server at proxy side
  // port : 7777
  if(end_serverfd < 0) {
    printf("connection failed\n");
    return;
  }

  /* 엔드 서버에 HTTP 요청 헤더를 보냄 */
  Rio_readinitb(&server_rio, end_serverfd);
  /* 엔드 서버에 http 헤더 씀 */
  Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

  /* 엔드 서버로부터 응답 메시지를 받아 클라이언트에 보내줌 */
  size_t n;
  while((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
    printf("proxy received %d bytes, then send\n", n);
    Rio_writen(connfd, buf, n);
  }
  Close(end_serverfd);
}

/*
 * buil_http_header - 클라이언트로부터 받은 용청 헤더를 process해서 프록시 서버가 엔드 서버에 보낼 요청 헤더 만듦.
 * 클라이언트 request가 "GET http://localhost:7777/home.html HTTP/1.1" 일떄 변수들:
 * request_hdr : "GET /home.html HTTP/1.0\r\n"
 * host_hdr : "Host : localhost:7777"
 * conn_hdr : "Connection : close\r\n"
 * prox_hdr : "Proxy-connection : close\r\n"
 * user_agent_hdr : Connection, Proxy-connection, User-agent가 아닌 모든 헤더
 * 다 저장하고 http_header에 전부 넣는다
*/
void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio)
{
  char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];

  /* 응답 라인 만들기 */
  sprintf(request_hdr, requestLint_hdr_format, path);

  /* 클라이언트 요청 헤더들에서 Host header와 나머지 header들을 구분해서 넣어준다 */
  /* 다른 헤더들 가져와서 변환 */
  while(Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
    if(strcmp(buf, endof_hdr) == 0) break; /*EOF, '\r\n' 만나면 끝*/

    /* 호스트 헤더 찾기 */
    if(!strncasecmp(buf, host_key, strlen(host_key))) /* Host: ~~ */ //일치하는게 있으면 0
    {
      strcpy(host_hdr, buf);
      continue;
    }

    if(strncasecmp(buf, connection_key, strlen(connection_key))
        && strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
        && strncasecmp(buf, user_agent_key, strlen(user_agent_key))) {
          strcat(other_hdr, buf);
        }
  }
  if(strlen(host_hdr) == 0) {
    sprintf(host_hdr, host_hdr_format, hostname);
  }

  /* 프록시 서버가 엔드 서버로 보낼 요청 헤더 작성 */
  sprintf(http_header, "%s%s%s%s%s%s%s",
          request_hdr,
          host_hdr,
          conn_hdr,
          prox_hdr,
          user_agent_hdr,
          other_hdr,
          endof_hdr);

  return;
}

/* 프록시 서버와 엔드 서버를 연결한다 */
inline int connect_endServer(char *hostname, int port, char *http_header)
{
  char portStr[100];
  sprintf(portStr, "%d", port);
  return Open_clientfd(hostname, portStr);
}

/* GET http://localhost:7777/home.html HTTP/1.1 일 때 
 * hostname : localhost
 * path : /home.html
 * port : 7777
*/
/* uri 파싱 -> hostname, path, port */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
  *port = 80; // 기본 설정
  char *pos = strstr(uri, "//"); // http:// 이후의 string
  
  pos = pos != NULL? pos + 2 : uri; // http:// 없어도 가능

  /* port와 path를 파싱 */
  char *pos2 = strstr(pos, ":");
  if(pos2 != NULL) {
    *pos2 = '\0';
    sscanf(pos, "%s", hostname);
    sscanf(pos2 + 1, "%d%s", port, path); // 기본 80에서 클라이언트 지정 포트로 변환
  }
  else {
    pos2 = strstr(pos, "/");
    if(pos2 != NULL) {
      *pos2 = '\0';
      sscanf(pos2, "%s", hostname);
      *pos2 = '/';
      sscanf(pos2, "%s", path);
    }
    else {
      sscanf(pos, "%s", hostname);
    }
  }
  return;
}