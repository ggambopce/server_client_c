#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // read, write, close
#include <sys/socket.h> // socket, bind, listen, accept
#include <arpa/inet.h>  // sockaddr_in, htons, inet_ntoa
#include "mysql.h"

#define BUF_SIZE 1024 // 수신 버퍼 크기
void error_handling(char *message);

// extern이란 여기는 없지만 다른파일 어딘가에는 있다는 뜻.
// 아래 4개 함수는 ControlDB.c 파일에 구현되어 있다. 
extern int initDB(MYSQL*, const char * host, const char * id, const char * pw, const char * db);
extern int writeDB(MYSQL*, int door, int gas, int flame, int fan, int pin);
extern int readDB(MYSQL*, char * buff, int size, int idRow);
extern int closeDB(MYSQL *);

// 아두이노 - 라즈베리파이 간 TCP서버
int main(int argc, char *argv[])
{
    // DB 연결
    MYSQL mysql;
    if (initDB(&mysql, "localhost", "root", "1234", "plantdb") < 0) {
        printf("(!) DB 연결 실패\n");
        exit(1);
    }

    int server_socket; // 연결 대기 전용 서버 소켓 식별자
    int client_socket; // 연결된 클라이언트 소켓 정보를 담을 변수

    char message[BUF_SIZE]; // 버퍼만큼 수신 데이터를 저장할 변수
    int string_length;      // read, rece등 반환값인 문자열 길이를 받을 변수

    struct sockaddr_in server_address; // 서버자신의 IP/포트 정보를 저장, bind()에서 사용
    struct sockaddr_in client_address; // 접속해 온 클라이언트의 IP/포트 정보를 저장 accept 사용
    socklen_t client_address_size;     // 넘어온 클라이언트 구조체 길이를 받을 정수값 변수 선언

    // 포트 번호 하나만 요구, 잘못 입력시 사용법 안내
    if (argc != 2)
    {
        printf("사용법 : %s <port>\n", argv[0]);
        exit(1);
    }

    // 1. 소캣생성 프로토콜체계:IPv4, 통신타입:TCP, 프로토콜기본값: 0
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
        error_handling("소캣 생성 실패");

    // 2. 연결 대기 서버 소캣 초기화 주소체계 설정, POSIX 표준 초기화 방식
    memset(&server_address, 0, sizeof(server_address)); // 메모리 시작위치, 초기화할 값, 초기화할 바이트 수
    server_address.sin_family = AF_INET;                // IPv4 주소체계 설정
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 서버의 모든 IP주소로부터의 요청을 수락 32비트 정수 빅엔디안
    server_address.sin_port = htons(atoi(argv[1]));     // 포트 번호를 16비트 정수 빅엔디안으로 변경

    // 3. 바인딩: 연결 대기 서버소켓에 IP/PORT 할당
    // 함수 원형 int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    // (socket()결과 소켓파일 디스크립터, 바인딩할 주소정보를 담고 있는 구조체 포인터, 주소 구조체의 크기)
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        error_handling("바인딩 실패");

    // 4. 클라이언트 접속 대기 상태
    // 함수 원형 int listen(int sockfd, int backlog)
    // (소캣파일 디스크립터, 연결 요청 커널 큐 최대 개수(수신 대기열 길이))
    if (listen(server_socket, 5) == -1)
        error_handling("리스닝 실패");
    else
        printf("서버가 시작되었습니다. 포트 %d에서 대기 중...\n", atoi(argv[1]));

    // 5.클라이언트 접속 수락
    client_address_size = sizeof(client_address); // 클라이언트 구조체 길이 받아올 준비

    // 함수 원형 int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    // (listen()중인 서버소캣파일 디스크립터, 연결요청 온 클라이언트 주소정보를 담고있는 구조체 포인터, 주소 구조체 크기 포인터)
    // 블로킹 호출, 연결요청이 수락되면 통신 전용 소캣 디스크립터 반환 - 통신 소캣 생성(client_socket)
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
    if (client_socket == -1)
        error_handling("클라이언트 연결 수락 실패");
    else
        printf("클라이언트 연결 성공 %s \n", inet_ntoa(client_address.sin_addr));

    // 6. 데이터 통신 서비스 로직
    while ((string_length = read(client_socket, message, BUF_SIZE)) != 0)
    {
        message[string_length] = '\0';                  // 수신된 문자열에 끝에 널 종료 문자 삽입
        printf("수신된 센서 원본 데이터: %s", message); // 원본메세지 콘솔에 출력

        // JSON 구조화를 위한 변수 선언
        double temp = 0.0, hum = 0.0;
        char *temp_ptr = strstr(message, "TEMP="); // TEMP= 키워드가 처음 등장하는 위치의 포인터
        char *hum_ptr = strstr(message, "HUM=");   // HUM= 키워드가 처음 등장하는 위치의 포인터

        // TEMP, HUM 모두 존재할 경우에만 파싱 시도
        if (temp_ptr && hum_ptr)
        {
            // 함수원형 int sscanf(const char *str, const char *format, ...);
            // (분석할 문자열 주소값, 추출할 형식 지정문자열, 실제데이터 저장할 변수의 포인터)
            sscanf(temp_ptr, "TEMP=%lf", &temp); // TEMP 값 추출
            sscanf(hum_ptr, "HUM=%lf", &hum);    // HUM 값 추출

            // 추출한 값을 JSON 형식으로 출력
            printf("파싱된 JSON: { \"temperature\": %.2f, \"humidity\": %.2f }\n", temp, hum);

            // DB에 저장 (device_id는 예시로 1번 사용)
            int device_id = 1;
            writeDB(&mysql, temp, hum, device_id);

        }
        else
        {
            // TEMP 또는 HUM 키가 누락된 경우 경고 출력
            printf("TEMP 또는 HUM 포맷 오류\n");
        }

        // 클라이언트에게 에코 응답보내기
        write(client_socket, message, string_length);
    }

    // 7. 자원 해제
    close(client_socket);
    close(server_socket);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr); // 출력할 문자열과 출력 대상 파일 스트림
    fputc('\n', stderr);    // 줄바꿈 문자 하나 출력
    exit(1);
}