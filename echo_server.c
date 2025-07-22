#include <stdio.h>                  // 표준 입출력 함수 사용(printf, perror)
#include <stdlib.h>                 // 일반 함수 사용(exit)
#include <string.h>                 // 문자열 관련 함수 사용(memset)
#include <unistd.h>                 // UNIX 표준 함수 사용(read, write, close)
#include <sys/socket.h>             // 소켓 관련 구조와 함수 정의
#include <arpa/inet.h>              // IP 주소 변환 함수 사용(inet_addr, htons)

/*
* TCP 기반 서버 소켓 생성
* 5명의 클라이언트까지 순차적으로 연결 수락
* 각 클라이언트가 보낸 데이터를 그대로 다시 돌려주는 에코 기능
*/
#define BUF_SIZE 1024               // 일반적으로 통신에 가장많이 쓰이는 버퍼크기 read(), recv() 부담없이 처리
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    int clnt_sock;

    char message[BUF_SIZE];
    int str_len, i;

    struct sockaddr_in serv_addr, clnt_adr;
    socklen_t clnt_adr_sz;

    if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
		error_handling("bind() error");

	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

    clnt_adr_sz=sizeof(clnt_adr);

    for(i=0; i<5; i++)
    {
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(clnt_sock==-1)
            error_handling("accept() error");
        else   
            printf("Connected client %d \n", i+1);
        
        while((str_len=read(clnt_sock, message, BUF_SIZE))!=0)
            write(clnt_sock, message, str_len);

        close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}