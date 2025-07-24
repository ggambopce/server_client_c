#include <stdio.h> 
#include <string.h> 
#include <errno.h> 
#include <wiringPi.h> 
#include <wiringSerial.h> 
#include <stdlib.h> 
#include "mysql.h" 
#include <time.h>

// MySQL DB를 초기화 하고 사용할 수 있도록 연결하는 함수
int initDB(MYSQL * mysql, const char * host, const char * id, const char * pw, const char * db) 
{
    printf("(i) initDB called, host=%s, id=%s, pw=%s, db=%s\n", host, id, pw, db); mysql_init(mysql); // DB 초기화
    if(mysql_real_connect(mysql, host, id, pw, db,0,NULL,0)) // DB 접속 
    { 
        printf("(i) mysql_real_connect success\n"); 
        return 0; // 성공 
    }
    printf("(!) mysql_real_connect failed\n");
    return -1; // 실패 
}

// DB에 쓰는 함수 - 정수형 인자 5개는 DB Table의 각 field라고 가정
int writeDB(MYSQL * mysql, int door, int gas, int flame, int fan, int pin) 
{ 
    char strQuery[255]=""; // 쿼리 작성에 사용할 버퍼
    // 삽입 쿼리 작성, time 필드는 DATE 타입의 현재 시각 
    sprintf(strQuery, "INSERT INTO twoteam(id, door, gas, flame, fan, pin, time) values(null, %d, %d, %d, %d, %d, now())", door, gas, flame, fan, pin);
    int res = mysql_query(mysql, strQuery); // 삽입 쿼리의 실행
    if (!res) // 성공 
    { 
        printf("(i) inserted %lu rows.\n", (unsigned long)mysql_affected_rows(mysql)); 
    } 
    else // 실패 
    { 
        fprintf(stderr, "(!) insert error %d : %s\n", mysql_errno(mysql), mysql_error(mysql)); return -1; 
    }
    return 0; 
}

// DB에서 읽어오기, id 가 primary key이고 읽은 결과는 buf에 구분자|를 이용해 반홖한다.
int readDB(MYSQL * mysql, char * buf, int size, int id) 
{ 
    char strQuery[256]="";          // select query를 작성할 버퍼 
    buf[0]=0; 
                          // 반환할 값은 strcat() 함수를 이용할 수 있도록 첫 바이트에 null을 넣는다.
    // select query 작성 
    sprintf(strQuery, "SELECT door, gas, flame, fan FROM twoteam WHERE id=%d;",id);
    int res = mysql_query(mysql, strQuery); // query의 실행
    if(res!=0) // 실패 
    { 
        return -1; 
    } 
    else // 성공 
    {
        // select 문의 처리 결과는 커서 형태로 시스템에 의해 관리된다. 
        MYSQL_RES * res_ptr = mysql_use_result(mysql);      // 시스템에 결과셋을 맡긴다. 
        MYSQL_ROW sqlrow = mysql_fetch_row(res_ptr);        // 결과셋에서 한줄만 받아온다.
        // 위에서 한줄만 가져오는 이유는 결과가 유일하다는 젂제에서 이다.
        // 만약 결과가 여러개일 경우 반복문을 돌면서 모든 row에 대해 처리해 주어야 한다.
        // make a string for return 
        unsigned int field_count=0; 
        while(field_count<mysql_field_count(mysql))         // 가져온 한줄의 모든 필드 값을 꺼내온다. 
        { 
            char buf_field[256]="";                         // 각 필드에 보관된 개별 값을 저장할 임시 버퍼 
            if(sqlrow[field_count]) 
                sprintf(buf_field,"|%s", sqlrow[field_count]); // 각 필드의 값을 문자열 형태로 가져온다. 
                else sprintf(buf_field,"|0");               // 값이 null 인 경우 0으로 표시한다.
            
            strcat(buf,buf_field);                          // 가져온 필드의 값을 반홖버퍼에 붙인다.(string append)
            field_count++;                                  // 다음 필드로 계속 진행 
        }

        if(mysql_errno(mysql)) // 만약 에러가 있었으면 
        { 
            fprintf(stderr, "(!) error: %s\n", mysql_error(mysql)); // 원인 표시 
            return -1; // 에러값 반홖 
        }
        mysql_free_result(res_ptr); // 시스템에 맡겨놓았던 결과셋을 이제 버리라고 한다. 
    }
    return 0; // 정상종료 반홖 
}

// DB 접속 종료하기
int closeDB(MYSQL * mysql) 
{ 
    mysql_close(mysql); // db 연결 해제 
    return 1; 
}