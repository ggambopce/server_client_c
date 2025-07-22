#include <stdio.h>                  // 표준 입출력 함수 사용(printf, perror)
#include <stdlib.h>                 // 일반 함수 사용(exit)
#include <string.h>                 // 문자열 관련 함수 사용(memset)
#include <unistd.h>                 // UNIX 표준 함수 사용(read, write, close)
#include <sys/socket.h>             // 소켓 관련 구조와 함수 정의
#include <arpa/inet.h>              // IP 주소 변환 함수 사용(inet_addr, htons)

/*
* TCP 기반 서버 소켓 생성
* 1.서버 소캣 생성
* 2.주소 구조체 설정 후 bind
* 3.listen으로 연결 대기열 생성
* 4.accept로 클라이언트 연결 수락
* 5.read/write로 에코처리
* 6.연결 종료 자원해제
* 5명의 클라이언트까지 순차적으로 연결 수락(동시처리 아님 acccept -> 처리 -> close반복)
* 각 클라이언트가 보낸 데이터를 그대로 다시 돌려주는 에코 기능
*/
#define BUF_SIZE 1024               // 일반적으로 통신에 가장많이 쓰이는 버퍼크기 read(), recv() 부담없이 처리
void error_handling(char *message);

int main(int argc, char *argv[])    // 넘어오는 인자의 갯수와 문자열 포인터들의 배열의 포인터
{
    int serv_sock;                  // 연결 대기 전용 서버 소켓 식별자
    int clnt_sock;                  // 클라이언트 소켓

    char message[BUF_SIZE];         // 데이터를 주고받을 버퍼    
    int str_len, i;                 // 문자열 길이와 반복문용 상수

    /* 대략 이렇게 생김
        struct sockaddr_in {              // IPv4용 주소정보를 저장하기 위한 구조체 
            sa_family_t     sin_family;   // 주소 체계 (IPv4이면 AF_INET)
            in_port_t       sin_port;     // 포트 번호 (2바이트, 네트워크 바이트 순서)
            struct in_addr  sin_addr;     // IP 주소 (4바이트)
            char            sin_zero[8];  // 사용 안 함 (패딩용), 관례
        };

     */
    struct sockaddr_in serv_addr;    // 서버자신의 IP/포트 정보를 저장 bind() 사용                
    struct sockaddr_in clnt_adr;     // 접속해 온 클라이언트의 IP/포트 정보를 저장 accept 사용 
    socklen_t clnt_adr_sz;           // 클라이언트의 주소 구조체 크기 accept()에서 사용
		
		// 포트 번호 하나만 요구, 잘못 입력시 사용법 안내
    if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// 1. 소켓 생성 Pv4(PF_INET), TCP(SOCK_STREAM), 프로토콜 0(자동 선택: IPPROTO_TCP)
  serv_sock=socket(PF_INET, SOCK_STREAM, 0); // 유닉스 계열OS에서 지원, 반환 값이 정수
	if(serv_sock == -1)
		error_handling("socket() error");

  // 2. 연결 대기 서버 소켓 초기화 POSIX 표준 초기화 방식
  memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;                // IPv4 주소체계 설정
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY); // 서버의 모든 IP주소로부터의 요청을 수락 32비트 정수 빅엔디안으로 변경
	serv_addr.sin_port=htons(atoi(argv[1]));     // 포트 번호를 16비트 정수 빅엔디안으로 변경

	// 3. 바인딩: 서버 대기 소켓에 IP/PORT 할당
	/**
	* 운영체젝가 내부적으로 반환한 정수 소켓 식별자에 서버 주소와 크기를 바인딩하고 성공시 0 반환
	* 함수 원형 int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
  * 형변환을 통해 범용 타입으로 변경 - 범용 구조체 포인터를 받도록 정의 되어있다.
  * 인터페이스처럼 다형성을 흉내내기 위한 수단
	*/
  if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
		error_handling("bind() error");

  // 4. 수신 대기 상태 진입 대기열 크기(커널 큐). 여러 클라이언트 동시 접속 대비.
	if(listen(serv_sock, 5)==-1)         // 함수 원형 int listen(int sockfd, int backlog);
		error_handling("listen() error");
		
  clnt_adr_sz=sizeof(clnt_adr);        // accept시 넘길 주소 길이 초기 크기값 공간 준비

  // 최대 5명의 클라이언트를 순차 처리
  for(i=0; i<5; i++)
  {
      // 5. 연결 수락: 블로킹 호출. 연결되면 통신용 소캣 반환
      clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz); // 연결된 클라이언트 소캣 식별자, 여러개 생길수 있다.
      if(clnt_sock==-1)
          error_handling("accept() error");
      else   
          printf("Connected client %d \n", i+1);
      
      // 6. 데이터 통신 서비스 로직
      /*
      * 클라이언트의 소캣에서 write() 호출이 TCP를 통해 운영체제 커널의 버퍼로 전달
      * 서버는 clnt_sock read()로 데이터 확인
      * read()와 write()는 반환값을 이벤트처럼 사용하여 데이터를 처리하게 된다.
      * 반환값n바이트만큼 처리
      */
      while((str_len=read(clnt_sock, message, BUF_SIZE))!=0)
          write(clnt_sock, message, str_len);

      close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);             // 출력할 문자열과 출력 대상 파일 스트림
	fputc('\n', stderr);                // 줄바꿈 문자 하나 출력
	exit(1);
}

/*
int fputs(const char *str, FILE *stream)로 보는 char 사용
| 표현           | 의미                           |
| ------------- | ---------------------------- |
| `char`        | 문자 하나                        |
| `char[]`      | 문자들의 배열 (문자열)                |
| `char*`       | 문자 배열의 시작 주소 (문자열을 가리키는 포인터) |
| `const char*` | 읽기 전용 문자열 포인터                |

* 포인터 만으로는 해당 자료의 크기를 알수 없다.
* sizeof도 포인터의 크기만 반환
* 포인터가 가리키는 자료형의 길이를 함께 넣어줘야 한다.
 */