#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>     // read, write, close
#include <sys/socket.h> // socket, bind, listen, accept
#include <arpa/inet.h>  // sockaddr_in, htons, inet_ntoa

#define BUF_SIZE 1024
void error_handling(char *message);

// 아두이노 - 라즈베리파이 간 TCP클라이언트
int main(int argc, char *argv[])
{
    int client_socket;      //  클라이언트 소켓파일 디스크립터
    char message[BUF_SIZE]; // 버퍼만큼 수신 데이터를 저장할 문자 배열 변수
    int string_length;      // read, rece등 반환값인 문자열 길이를 받을 변수

    struct sockaddr_in server_address; // 서버 주소 정보를 담는 구조체

    // 실행 인자가 4개가 아닌 경우, 사용법 안내 후 종료
    if (argc != 4)
    {
        printf("사용법 : %s <IP> <port> <device_id>\n", argv[0]);
        exit(1);
    }

    // 1. 소캣생성 프로토콜체계:IPv4, 통신타입:TCP, 프로토콜기본값: 0
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
        error_handling("소캣 생성 실패");

    // 2. 연결하고 싶은 서버 주소 초기화 주소체계 설정
    memset(&server_address, 0, sizeof(server_address));  // 메모리 시작위치, 초기화할 값, 초기화할 바이트 수
    server_address.sin_family = AF_INET;                 // IPv4 주소체계 설정
    server_address.sin_addr.s_addr = inet_addr(argv[1]); // 명령줄 인자로 전달된 IP주소
    server_address.sin_port = htons(atoi(argv[2]));      // 명령줄 인자로 전달된 포트 번호를 16비트 정수 빅엔디안으로 변경

    // 3. 서버에 연결 요청xxx
    // 함수 원형 int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    //(클라이언트 소캣파일 디스크립터, 연결하려는 서버 주소정보를 담고 있는 구조체 포인터, 서버 주소 구조체 크기)
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        error_handling("connect() error!");
    else
        puts("연결되었습니다........");

    // 4. 데이터 통신 서비스 로직
    // 랜덤 값 생성을 위한 시드 초기화
    srand((unsigned int)time(NULL)); // 현재 시간을 기반을 으로 난수 시드 설정

    // 데이터 전송 루프
    while (1)
    {
        int device_id = atoi(argv[3]);
        // 4-1. 랜덤 온도 및 습도 생성
        float temp = 20.0 + (rand() % 1000) / 100.0f; // TEMP: 20.0 ~ 29.99
        float hum = 40.0 + (rand() % 3000) / 100.0f;  // HUM: 40.0 ~ 69.99

        // 4-2. sprintf를 이용해 문자열로 포맷팅
        // TEMP=24.52,HUM=61.47\n 형식의 문자열 생성
        sprintf(message, "TEMP=%.2f,HUM=%.2f,ID=%d\n", temp, hum, device_id);

        // 5. 입력 메시지를 서버로 전송 (문자열 길이만큼)
        // 함수 원형 ssize_t write(int fd, const void *buf, size_t count);
        // (전송할 대상 소캣 디스크립터, 전송할 데이터가 들어있는 버퍼(배열포인터), 전송할 데이터 크기)
        // 실제로 전송할 바이트수 반환
        write(client_socket, message, strlen(message));

        // 6. 서버로부터 응답 수신 (최대 BUF_SIZE - 1)
        // 함수 원형 ssize_t read(int fd, void *buf, size_t count);
        // (읽을 대상 디스크립터, 읽어온 데이터 저장할 버퍼(배열포인터), 최대 읽을 수 있는 바이트수)
        // 실제로 읽은 바이트 수 반환
        string_length = read(client_socket, message, BUF_SIZE - 1);
        message[string_length] = '\0'; // 널 종료문자 의미, 문자배열 수동 \0 삽입

        // 7. 서버 응답 출력
        printf("서버 응답: %s", message);

        // 8. 1초 대기 후 다음 전송
        sleep(3);
    }

    // 8. 소켓 종료 (TCP 연결 종료)
    close(client_socket);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}