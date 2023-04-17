

# **tiny웹 서버**

# **tiny.c**

## **main()**


> **port 번호를 인자로 받아** 클라이언트의 요청이 올 때마다 새로 연결 소켓을 만들어 doit() 함수를 호출한다.
> 

```c
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* port 번호를 인자로 받는다. */
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;  // 클라이언트에서 연결 요청을 보내면 알 수 있는 클라이언트 연결 소켓 주소

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* 해당 포트 번호에 해당하는 듣기 소켓 식별자를 열어준다. */
  listenfd = Open_listenfd(argv[1]);

  /* 클라이언트의 요청이 올 때마다 새로 연결 소켓을 만들어 doit() 호출*/
  while (1) {
    /* 클라이언트에게서 받은 연결 요청을 accept한다. connfd = 서버 연결 식별자 */
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    
    /* 연결이 성공했다는 메세지를 위해. Getnameinfo를 호출하면서 hostname과 port가 채워진다. */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    /* doit 함수를 실행! */
    doit(connfd);   // line:netp:tiny:doit

    /* 서버 연결 식별자를 닫아준다. */
    Close(connfd);  // line:netp:tiny:close
  }
}
```

## **doit()**


> 클라이언트의 요청 라인을 확인해 정적, 동적 컨텐츠인지를 구분하고 각각의 서버에 보낸다.
> 

```c
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];  // 클라이언트에게서 받은 요청(rio)으로 채워진다.
  char filename[MAXLINE], cgiargs[MAXLINE];  // parse_uri를 통해 채워진다.
  rio_t rio;

  /* Read request line and headers */
  /* 클라이언트가 rio로 보낸 request 라인과 헤더를 읽고 분석한다. */
  Rio_readinitb(&rio, fd); // rio 버퍼와 fd, 여기서는 서버의 connfd를 연결시켜준다.
  Rio_readlineb(&rio, buf, MAXLINE); // 그리고 rio(==connfd)에 있는 string 한 줄(응답 라인)을 모두 buf로 옮긴다.
  printf("Request headers:\n");
  printf("%s", buf);  // 요청 라인 buf = "GET /godzilla.gif HTTP/1.1\0"을 표준 출력만 해줌!
  sscanf(buf, "%s %s %s", method, uri, version); // buf에서 문자열 3개를 읽어와 method, uri, version이라는 문자열에 저장한다.

  // 요청 method가 GET이 아니면 종료. main으로 가서 연결 닫고 다음 요청 기다림.
  if (strcasecmp(method, "GET")) {  // method 스트링이 GET이 아니면 0이 아닌 값이 나옴
    clienterror(fd, method, "501", "Not implemented",
    "Tiny does not implement this method");
    return;
  }

  // 요청 라인을 뺀 나머지 요청 헤더들을 무시(그냥 프린트)한다.
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  /* parse_uri : 클라이언트 요청 라인에서 받아온 uri를 이용해 정적/동적 컨텐츠를 구분한다. */
  is_static = parse_uri(uri, filename, cgiargs); // 정적이면 1

  /* stat(file, *buffer) : file의 상태를 buffer에 넘긴다. */
  /* 여기서 filename : 클라이언트가 요청한 서버의 컨텐츠 디렉토리 및 파일 이름? */
  if (stat(filename, &sbuf) < 0) {  // 못 넘기면 fail. 파일이 없다. 404.
    clienterror(fd, filename, "404", "Not found",
    "Tiny couldn't find this file");
    return;
  }

  /* 컨텐츠의 유형(정적, 동적)을 파악한 후 각각의 서버에 보낸다. */
  if (is_static) { /* Serve static content */
    // !(일반 파일이다) or !(읽기 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
      "Tiny couldn't read the file");
      return;
    }

    // 정적 서버에 파일의 사이즈를 같이 보낸다. -> Response header에 Content-length 위해!
    serve_static(fd, filename, sbuf.st_size);
  }
  else { /* Serve dynamic content */
    // !(일반 파일이다) or !(실행 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
      "Tiny couldn't run the CGI program");
      return;
      }

    // 동적 서버에 인자를 같이 보낸다.
    serve_dynamic(fd, filename, cgiargs);
  }
}
```

## **clienterror()**

> 에러 메세지와 응답 본체를 서버 소켓을 통해 클라이언트에 보낸다.
> 

```c
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));

  // 에러메세지와 응답 본체를 서버 소켓을 통해 클라이언트에 보낸다. 
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
```

## **read_requesthdrs**

> 클라이언트가 버퍼 rp에 보낸 나머지 요청 헤더들을 무시한다(그냥 프린트한다).
> 

```c
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);

  /* 버퍼 rp의 마지막 끝을 만날 때까지("Content-length: %d\r\n\r\n에서 마지막 \r\n) */
  /* 계속 출력해줘서 없앤다. */
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
```

## **parse_uri**

> uri를 받아 요청받은 파일의 이름(filename)과 요청 인자(cgiarg)를 채워준다.
> 

```c
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  /* uri에 cgi-bin이 없다면, 즉 정적 컨텐츠를 요청한다면 1을 리턴한다.*/
  // 예시 : GET /godzilla.jpg HTTP/1.1 -> uri에 cgi-bin이 없다
  if (!strstr(uri, "cgi-bin")) { /* Static content, uri안에 "cgi-bin"과 일치하는 문자열이 있는지. */
    strcpy(cgiargs, "");    // 정적이니까 cgiargs는 아무것도 없다.
    strcpy(filename, ".");  // 현재경로에서부터 시작 ./path ~~
    strcat(filename, uri);  // filename 스트링에 uri 스트링을 이어붙인다.

    // 만약 uri뒤에 '/'이 있다면 그 뒤에 home.html을 붙인다.
    // 내가 브라우저에 http://localhost:8000만 입력하면 바로 뒤에 '/'이 생기는데,
    // '/' 뒤에 home.html을 붙여 해당 위치 해당 이름의 정적 컨텐츠가 출력된다.
    if (uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");

    /* 예시
      uri : /godzilla.jpg
      ->
      cgiargs : 
      filename : ./godzilla.jpg
    */
    
    // 정적 컨텐츠면 1 리턴
    return 1;
  }
  else { /* Dynamic content */
    ptr = index(uri, '?');

    // '?'가 있으면 cgiargs를 '?' 뒤 인자들과 값으로 채워주고 ?를 NULL로 만든다.
    if (ptr) { 
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else // '?'가 없으면 그냥 아무것도 안 넣어준다.
      strcpy(cgiargs, "");

    strcpy(filename, ".");  // 현재 디렉토리에서 시작
    strcat(filename, uri);  // uri 넣어준다.

    /* 예시
      uri : /cgi-bin/adder?123&123
      ->
      cgiargs : 123&123
      filename : ./cgi-bin/adder
    */

    return 0;
  }
}
```

## **serve_static()**

> 클라이언트가 원하는 정적 컨텐츠 디렉토리를 받아온다. 응답 라인과 헤더를 작성하고 서버에게 보낸다. 그 후 정적 컨텐츠 파일을 읽어 그 응답 본체를 클라이언트에 보낸다.
> 

```c
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client 클라이언트에게 응답 헤더 보내기*/
  /* 응답 라인과 헤더 작성 */
  get_filetype(filename, filetype);  // 파일 타입 찾아오기 
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  // 응답 라인 작성
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);  // 응답 헤더 작성
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  
  /* 응답 라인과 헤더를 클라이언트에게 보냄 */
  Rio_writen(fd, buf, strlen(buf));  // connfd를 통해 clientfd에게 보냄
  printf("Response headers:\n");
  printf("%s", buf);  // 서버 측에서도 출력한다.

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // filename의 이름을 갖는 파일을 읽기 권한으로 불러온다.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 메모리에 파일 내용을 동적할당한다.
  Close(srcfd); // 파일을 닫는다.
  Rio_writen(fd, srcp, filesize);  // 해당 메모리에 있는 파일 내용들을 fd에 보낸다(읽는다).
  Munmap(srcp, filesize);
}
```

## **get_filetype()**

> filename을 조사해 각각의 식별자에 맞는 MIME 타입을 filetype에 입력해준다.
> 

Response header의 Content-type에 필요하다.

```c
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))  // filename 스트링에 ".html" 
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}
```

## **serve_dynamic()**

> 클라이언트가 원하는 **동적 컨텐츠 디렉토리를 받아온다. 응답 라인과 헤더를 작성**하고 **서버에게 보낸다.** CGI 자식 프로세스를 **fork**하고 그 프로세스의 **표준 출력을 클라이언트 출력과 연결**한다.
> 

```c
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };
  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);  // 

    // 클라이언트의 표준 출력을 CGI 프로그램의 표준 출력과 연결한다.
    // 이제 CGI 프로그램에서 printf하면 클라이언트에서 출력됨
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}
```

**for문의 내용 이해하기**

```c
  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);  // 

    // 클라이언트의 표준 출력을 CGI 프로그램의 표준 출력과 연결한다.
    // 이제 CGI 프로그램에서 printf하면 클라이언트에서 출력됨
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
```

- `fork()`를 실행하면 부모 프로세스와 자식 프로세스가 **동시에 실행**된다.
- 만약 `fork()`의 반환값이 0 이라면, 즉 자식 프로세스라면 `if 문`을 수행한다.
- `fork()`의 반환값이 0이 아니라면, 즉 부모 프로세스라면 `if 문`을 건너뛰고 `Wait(NULL)` 함수로 간다. 이 함수는 부모 프로세스가 먼저 도달해도 자식 프로세스가 종료될 때까지 기다리는 함수이다.
- `if 문` 안에서 `setenv` 시스템 콜을 수행해 `“QUERY_STRING”`의 값을 `cgiargs`로 바꿔준다. 우선 순위가 `1`이므로 기존의 값과 상관없이 값이 변경된다.
- `dup2` 함수를 실행해서 CGI 프로세스의 표준 출력을 `fd`(서버 연결 소켓 식별자)로 복사한다. 이제 `STDOUT_FILENO`의 값은 `fd`이다. 다시 말해, CGI 프로세스에서 표준 출력을 하면 그게 서버 연결 식별자를 거쳐 클라이언트에 출력된다.
- `execuv` 함수를 이용해 파일 이름이 `filename`인 파일을 실행한다.

<br>

# **/cgi-bin/adder.c**

```c
#include "csapp.h"

int main(){
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    /* 서버 프로세스가 만들어준 QUERT_STRING 환경변수를 getenv로 가져와 buf에 넣음 */
    if((buf = getenv("QUERY_STRING")) != NULL){
        p = strchr(buf, '&'); // 포인터 p는 &가 있는 곳의 주소
        *p = '\0';      // &를 \0로 바꿔주고
        strcpy(arg1, buf);  // & 앞에 있었던 인자
        strcpy(arg2, p+1);  // & 뒤에 있었던 인자
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    /* content라는 string에 응답 본체를 담는다. */
    sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);  // ""안의 string이 content라는 string에 쓰인다.
    sprintf(content, "%sWelcome to add.com: ", content);
    sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1+n2);  // 인자를 받아 처리를 해줬다! -> 동적 컨텐츠
    sprintf(content, "%sThanks for visiting!\r\n", content);

    /* CGI 프로세스가 부모인 서버 프로세스 대신 채워 주어야 하는 응답 헤더값*/
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Conten-type: text/html\r\n\r\n");
    printf("%s", content);  // 응답 본체를 클라이언트 표준 출력!
    fflush(stdout);

    exit(0);
}
```
<br>

# **결과**

## **정적 컨텐츠**

```bash
http://3.141.45.244:7777/godzilla.jpg
-> 
cgiargs : 
filename : ./godzilla.jpg

http://3.141.45.244:7777/
->
cgiargs : 
filename : ./home.html

```

**클라이언트 요청**

```c
GET / HTTP/1.1
```

**서버 출력**

```c
Accepted connection from (localhost, 36296)
Request headers:
GET / HTTP/1.1
Response headers:
HTTP/1.0 200 OK
Server: Tiny Web Server
Connection: close
Content-length: 120
Content-type: text/html
```

**클라이언트 출력**

```c
HTTP/1.0 200 OK
Server: Tiny Web Server
Connection: close
Content-length: 120
Content-type: text/html

<html>
<head><title>test</title></head>
<body> 
<img align="middle" src="godzilla.gif">
Dave O'Hallaron
</body>
</html>
```

## **동적 컨텐츠**

```bash
http://3.141.45.244:7777/cgi-bin/adder?123&123
->
cgiargs : 123&123
filename : ./cgi-bin/adder
```

**클라이언트 요청**

```c
GET /cgi-bin/adder?123&123 HTTP/1.1
```

**서버 출력**

```c
Request headers:
GET /cgi-bin/adder?123&123 HTTP/1.1
```

**클라이언트 출력**

```c
HTTP/1.0 200 OK
Server: Tiny Web Server
Connection: close
Content-length: 133
Conten-type: text/html

QUERY_STRING=123
<p>Welcome to add.com: THE Internet addition portal. 
<p>The answer is: 123 + 123 = 246
<p>Thanks for visiting!
```
<br>

## **숙제 11.7**

MPG 비디오 파일을 처리하도록 하시오.

### **get_filetype()**

mp4 파일 형식에 대응하는 MIME 타입을 만들어준다.

```c
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))  // filename 스트링에 ".html" 
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}
```

### **home.html**

html에 넣어보았다.

```c
<html>
<head><title>test</title></head>
<body> 
<img align="middle" src="godzilla.gif">
Dave O'Hallaron
<video width="350" height="500" controls>
    <source src="quokka.mp4" type="video/mp4">
    Your browser does not support the video tag.
</video>
</body>
</html>
```

## **11.9**

Tiny를 수정해서 정적 컨텐츠를 처리할 때 요청한 파일을 mmap과 rio_readn 대신에 malloc, rio_readn, rie_writen을 사용해서 연결 식별자에게 복사하도록 하시오.

### **serve_static()**

- 파일의 메모리(데이터)를 그대로 가상 메모리에 매핑하는 `mmap()`과 달리, 먼저 **파일의 크기만큼 메모리를 동적 할당 해준 뒤** `rio_readn`을 사용하여 **파일의 데이터를 메모리로 읽어와야** 한다.

```c
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client 클라이언트에게 응답 헤더 보내기*/
  /* 응답 라인과 헤더 작성 */
  get_filetype(filename, filetype);  // 파일 타입 찾아오기 
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  // 응답 라인 작성
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);  // 응답 헤더 작성
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  
  /* 응답 라인과 헤더를 클라이언트에게 보냄 */
  Rio_writen(fd, buf, strlen(buf));  // connfd를 통해 clientfd에게 보냄
  printf("Response headers:\n");
  printf("%s", buf);  // 서버 측에서도 출력한다.

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // filename의 이름을 갖는 파일을 읽기 권한으로 불러온다.
  srcp = (char*)Malloc(filesize);  // 파일 크기만큼의 메모리를 동적할당한다.
  Rio_readn(srcfd, srcp, filesize);   // filename 내용을 동적할당한 메모리에 쓴다.
  Close(srcfd); // 파일을 닫는다.
  Rio_writen(fd, srcp, filesize);  // 해당 메모리에 있는 파일 내용들을 클라이언트에 보낸다(읽는다).
  free(srcp);
}
```

## **11.10**

**A.** CGI adder 함수에 대한 HTML 형식을 작성하시오. 여러분의 형식은 GET 메소드를 사용해서 컨텐츠를 요청해야 한다.

**B.** 실제 브라우저를 사용해서 Tiny로부터 이 형식을 요청하고, 채운 형식을 Tiny에 보내고, adder가 생성한 동적 컨텐츠를 표시하는 방법으로 여러분의 작업을 체크하라.

### **form-adder.html**

action form에 input을 채우고 submit을 누르면, 해당 `name=text`을 URL 인자로 보내준다.

```html
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8" />
        <title>Tiny Server</title>
    </head>
    <body>
        <form action="/cgi-bin/form-adder" method="GET">
        <p>first number: <input type="text" name="first"/></p>
        <p>second number: <input type="text" name="second"/></p>
        <input type="submit" value="Submit"/>
        </form>
    </body>
</html>
```

### **/cgi-bin/form-adder.c**

```c
#include "csapp.h"

int main(){
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    /* 인자 2개 추출하기 */
    if ((buf = getenv("QUERY_STRING")) != NULL)
    {
        p = strchr(buf, '&');
        *p = '\0';
        sscanf(buf, "first=%d", &n1); // buf에서 %d를 읽어서 n1에 저장
        sscanf(p+1, "second=%d", &n2);
    }

    /* 응답 본체 만들기 */
    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
        content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    /* 클라이언트에 보낼 응답 출력하기 */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
```
<p align = 'center'>
   <img src = 'https://user-images.githubusercontent.com/93521799/147042399-2d9b8f93-5e35-404c-99b5-9ba84b125ef3.png'>
</p>
<p align = 'center'>
   <em>이렇게 input을 채우고 submit을 누르면</em>
</p>


```html
http://3.141.45.244:7777/cgi-bin/form-adder?first=13&second=5
이렇게 해당 URL에 인자를 더해준다.
```

## **11.11**

Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. Telnet으로 작업 결과를 체크하시오.

> HEAD 메소드는 GET과 유사하지만, 서버가 HTTP Response 메세지의 본체를 전송하지 않는다.
> 

## **tiny.c**

**doit()**

```c
	// 요청 method가 GET과 HEAD가 아니면 종료. main으로 가서 연결 닫고 다음 요청 기다림.
  if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {  // method 스트링이 GET이 아니면 0이 아닌 값이 나옴
    clienterror(fd, method, "501", "Not implemented",
    "Tiny does not implement this method");
    return;
  }

...

	/* 컨텐츠의 유형(정적, 동적)을 파악한 후 각각의 서버에 보낸다. */
  if (is_static) { /* Serve static content */
    // !(일반 파일이다) or !(읽기 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
      "Tiny couldn't read the file");
      return;
    }

    // 정적 서버에 파일의 사이즈와 메서드를 같이 보낸다. -> Response header에 Content-length 위해!
    serve_static(fd, filename, sbuf.st_size, method);
  }
  else { /* Serve dynamic content */
    // !(일반 파일이다) or !(실행 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
      "Tiny couldn't run the CGI program");
      return;
      }

    // 동적 서버에 인자와 메서드를 같이 보낸다.
    serve_dynamic(fd, filename, cgiargs, method);
  }
}
```

**serve_static()**

```c
	/* 응답 라인과 헤더를 클라이언트에게 보냄 */
  Rio_writen(fd, buf, strlen(buf));  // connfd를 통해 clientfd에게 보냄
  printf("Response headers:\n");
  printf("%s", buf);  // 서버 측에서도 출력한다.

  /* 만약 메서드가 HEAD라면, 응답 본체를 만들지 않고 끝낸다.*/
  if (strcasecmp(method, "HEAD") == 0)
    return;

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // filename의 이름을 갖는 파일을 읽기 권한으로 불러온다.
  srcp = (char*)Malloc(filesize);  // 파일 크기만큼의 메모리를 동적할당한다.
  Rio_readn(srcfd, srcp, filesize);   // filename 내용을 동적할당한 메모리에 쓴다.
  Close(srcfd); // 파일을 닫는다.
  Rio_writen(fd, srcp, filesize);  // 해당 메모리에 있는 파일 내용들을 클라이언트에 보낸다(읽는다).
  free(srcp);
}
```

**serve_dynamic()**

```c
if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);

    /* 요청 메서드를 환경변수에 추가한다. */
    setenv("REQUEST_METHOD", method, 1);

    // clientfd 출력을 CGI 프로그램의 표준 출력과 연결한다.
    // CGI 프로그램에서 printf하면 클라이언트에서 출력됨!!
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
```

클라이언트 요청

```c
HEAD /cgi-bin/head-adder?123&456 HTTP/1.1
```

서버 응답

```c
Request headers:
HEAD /cgi-bin/head-adder?123&456 HTTP/1.1
```

클라이언트 출력

```c
HTTP/1.0 200 OK
Server: Tiny Web Server
Connection: close
Content-length: 133
Conten-type: text/html
```