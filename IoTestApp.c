#include <stdio.h> 
#include <string.h> 
#include <errno.h> 
#include <wiringPi.h> 
#include <wiringSerial.h> 
#include <stdlib.h> 
#include "mysql.h" 
#include <time.h>

// extern이란 여기는 없지만 다른파일 어딘가에는 있다는 뜻입니다.
// 아래 4개 함수는 ControlDB.c 파일에 구현되어 있습니다. 
extern int initDB(MYSQL*, const char * host, const char * id, const char * pw, const char * db); 
extern int writeDB(MYSQL*, int door, int gas, int flame, int fan, int pin); 
extern int readDB(MYSQL*, char * buff, int size, int idRow); 
extern int closeDB(MYSQL *);

int main() 
{
    printf("(i) This is a test program for the ControlDB.\n"); // 인삿말
    // DB에 삽입할 4가지 정수형 정보들
    int randomDoor = 0; 
    int randomGas = 0; 
    int randomFlame = 0; 
    int randomFan = 0;
    // 시드 랜덤값을 위해 시갂으로 초기화
    srand(time(NULL));
    
    // 4가지 정보는 테스트를 위해 랜덤으로 만들어 둔다.
    randomDoor = rand()%256; 
    randomGas = rand()%256; 
    randomFlame = rand()%256; 
    randomFan = rand()%256;

    // 잘 만들어 졌는지 한번 확인
    printf("(i) random: door=%d, gas=%d, flame=%d, fan=%d\n", randomDoor, randomGas, randomFlame, randomFan);
    
    MYSQL mysql; // mysql DB의 연결 상태를 관리할 구조체 변수 생성
    // DB에 연결해보자. 로컬에 접속하고, id는 root, 비번은 1234, DB명은 ssw 이다. 
    if(initDB(&mysql, "localhost","root","1234","ssw")<0) 
    { 
        printf("(!) initDB failed\n"); // 실패 
        return -1; 
    } 
    else printf("(i) initDB successd!\n"); // 성공

    int pin = 6; // 추가적으로 필요한 필드(별거 아님;)

    // DB에 쓰기 - 성공 시 0, 실패 시 -1 반환
    int res = writeDB(&mysql, randomDoor, randomGas, randomFlame, randomFan,pin); 
    if(res<0) 
    { 
        printf("(!) writeDB failed\n"); 
        return -1; 
    } else printf("(i) writeDB success!, \n");

    // 이제 삽입한 row의 값을 다시 읽어와 보자.
    char buf[256]=""; // 결과를 받아올 문자열 버퍼 
    int rowId = 1; // 테스트로 id가 1인 row의 값을 읽어온다. 
    res = readDB(&mysql, buf, 256, rowId); // 읽기 실행 
    if(res<0) // 실패 
    { 
        printf("(!) readDB failed\n"); 
        return -1; 
    } else printf("(i) readDB success!, buf=%s\n", buf); // 성공

    if(closeDB(&mysql)<0) // DB 연결 끊기 
    { 
        printf("(!) clseDB failed\n"); //실패 
        return -1; 
    } else printf("(i) closeDB success\n"); //성공

    // 종료 알림
    printf("(i) This program will be closed.\n");
    return 0; 
}