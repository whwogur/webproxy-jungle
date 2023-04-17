/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(){
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    /* 자식 프로세스가 따로 만든 QUERT_STRING 환경변수를 getenv로 가져와 buf에 넣음 */
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