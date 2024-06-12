#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

// 주식 데이터 구조체 선언
struct stock_data {
    char symbol[10];
    float open;
    float high;
    float low;
    float close;
    int volume;
};

// 주식 데이터를 분석하는 함수 선언
void analyze_stock(struct stock_data* stock);

// IPC 핸들링 함수 선언
void handle_ipc();

// 시그널 핸들러 함수
void signal_handler(int sig) {
    if (sig == SIGUSR1) { // SIGUSR1 시그널을 받으면 메시지를 출력합니다.
        printf("SIGUSR1 signal을 받았습니다.\n");
    }
}

// 메인 함수
int main() {
    // 시그널 핸들러 등록
    struct sigaction shoot;
    shoot.sa_handler = signal_handler;
    sigemptyset(&shoot.sa_mask);
    shoot.sa_flags = 0;

    if (sigaction(SIGUSR1, &shoot, NULL) == -1) {
        perror("시그널 받기 실패");
        return 1;
    }

    // 파일의 마지막 수정 날짜 확인
    struct stat attr;
    if (stat("nasdaq_stocks_6months.csv", &attr) != 0) {
        perror("파일 정보를 불러오는데 실패하였습니다.");
        return 1;
    }
    struct tm* mod_time = localtime(&attr.st_mtime);
    char mod_time_str[20];
    strftime(mod_time_str, sizeof(mod_time_str), "%Y-%m-%d %h %m %s", mod_time);

    // 현재 시간을 구함
    time_t current_time;
    time(&current_time);
    struct tm* now = localtime(&current_time);
    char now_str[20];
    strftime(now_str, sizeof(now_str), "%Y-%m-%d %h %m %s", now);

    printf("Last modified: %s\n", mod_time_str);
    printf("Current date: %s\n", now_str);

    // 파일 수정 날짜가 오늘인지 확인
    if (strcmp(mod_time_str, now_str) == 0) {
        printf("File was last modified today.\n");
    } else { // 만약 오늘이 아니라면 python 스크립트 실행
        printf("File was last modified on: %s\n", mod_time_str);
        printf("Running the update program...\n");

        // 자식 프로세스 생성
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0) {
            // Python 스크립트 실행
            char *args[] = {"/usr/bin/python3", "/home/seungbin/test/SystemProgramming/make_data.py", NULL};
            execvp(args[0], args);

            // execvp가 실패하면 에러 메시지 출력
            perror("execvp 실패하였습니다");
            exit(1);
        } else {
            // 부모 프로세스: 자식 프로세스 종료 대기
            wait(NULL);
            printf("자식 프로세스 종료 되었습니다.\n");

            // 주식 데이터를 읽고 분석
            FILE* file = fopen("nasdaq_stocks_6months.csv", "r");
            if (file == NULL) {
                perror("파일을 여는데 실패하였습니다");
                return 1;
            }

            struct stock_data stock;
            struct stock_data last_stock;
            int index = 0;

            // CSV 파일을 읽어 구조체에 주식 데이터를 저장
            while (fscanf(file, "%s,%f,%f,%f,%f,%d\n",
                          stock.symbol, &stock.open, &stock.high,
                          &stock.low, &stock.close, &stock.volume) != EOF) {
                last_stock = stock;  // 마지막 읽은 주식 데이터 저장
                index++;
            }

            fclose(file);

            // 주식 데이터 분석
            analyze_stock(&last_stock); // 가장 마지막으로 읽은 데이터 분석
        }
    }

    handle_ipc();

    // SIGUSR1 시그널을 부모 프로세스에 보냄
    kill(getpid(), SIGUSR1);

    return 0;
}
