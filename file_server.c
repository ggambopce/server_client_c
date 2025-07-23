#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 30                             // 바이너리 데이터 전송 확인용, 현업에서는 크게 사용
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sd;                                // 서버 소켓 식별자
    int clnt_sd;                                // 연결 후 생성되는 클라이언트연결 소켓 식별자
    FILE * fp;                                  // 전송할 파일을 가리키는 파일 포인터
    char buf[BUF_SIZE];                         // 전송에 사용할 버퍼
    int read_cnt;                               // fread로 읽은 바이트 수

    struct sockaddr_in serv_adr;                // 서버주소정보 구조체
    struct sockaddr_in clnt_adr;                // 클라이언트주소정보 구조체
    socklen_t clnt_adr_sz;                      // 

    if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    fp=fopen("file_server.c", "rb");
    serv_sd=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr*) &serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    clnt_adr_sz=sizeof(clnt_adr);
    clnt_sd=accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

    while(1)
    {
        read_cnt=fread((void*)buf, 1, BUF_SIZE, fp);
        if(read_cnt<BUF_SIZE)
        {
            write(clnt_sd, buf, read_cnt);
            break;
        }
        write(clnt_sd, buf, read_cnt);
    }

    shutdown(clnt_sd, SHUT_WR);
    read(clnt_sd, buf, BUF_SIZE);
    printf("Message from client: %s \n", buf);

    fclose(fp);
    close(clnt_sd);
    close(serv_sd);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}