#include <unistd.h>
#include <stdio.h>          // printf, puts
#include <pthread.h>        // pthread 생성, 종료대기, 상호배제,  

void* thread_main(void *arg);

int main(int argc, char *argv[])
{
    pthread_t t_id;
    int thread_param=5;


    if(pthread_create(&t_id, NULL, thread_main, (void*)&thread_param)!=0)
    {
        puts("pthread_create() error");
        return -1;
    };
    sleep(10);
    puts("end of mail");
    return 0;
}

// 새로 생성된 스레드의 시작 지점
// 1초에 한번씩 문자열을 출력하는 스레드 함수
void* thread_main(void *arg)
{
    int i;
    /*
    void*타입이라 직접 역참조함수 없기 때문에 int* 으로 형변환해서 정수형 포인터처럼 접근하여 정수값을 꺼냄
    */
    int cnt=*((int*)arg);           // 메인함수에서 넘겨준 정수 포인터를 역참조해서 반복횟수로 사용
    for(i=0; i<cnt; i++)
    {
        sleep(1);                   // 1초간 멈추기
        puts("running thread");     // 함수 원형 puts(const char *str) 문자열을 표준출력으로 반환
    }
    return NULL;
}