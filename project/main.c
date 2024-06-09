#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// 구조체 선언: 주식 데이터
struct stock_data {
    char symbol[10];
    float open;
    float high;
    float low;
    float close;
    int volume;
};

// analyze_stock 함수 선언
void analyze_stock(struct stock_data* stock);

int main() {
    // 파일의 마지막 수정 날짜 확인
    struct stat attr;
    stat("nasdaq_stocks_6months.csv", &attr);
    struct tm* mod_time = localtime(&attr.st_mtime);

    time_t current_time;
    time(&current_time);
    struct tm* now = localtime(&current_time);

    if (mod_time->tm_year == now->tm_year &&
        mod_time->tm_mon == now->tm_mon &&
        mod_time->tm_mday == now->tm_mday) {
        printf("File was last modified today.\n");
    } else {
        printf("File was last modified on: %d-%d-%d\n",
               mod_time->tm_year + 1900, mod_time->tm_mon + 1, mod_time->tm_mday);
        printf("Running the update program...\n");
        system("/usr/bin/python3 /home/seungbin/test/project/make_data.py");

        // 자식 프로세스 생성
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0) {
            // 자식 프로세스: 주식 데이터를 읽고 분석
            FILE* file = fopen("nasdaq_stocks_6months.csv", "r");
            if (file == NULL) {
                perror("Failed to open file");
                exit(1);
            }

            struct stock_data stocks[100]; // 배열 선언
            int index = 0;

            // CSV 파일을 읽어 구조체 배열에 주식 데이터를 저장
            while (fscanf(file, "%10[^,],%f,%f,%f,%f,%d\n",
                          stocks[index].symbol, &stocks[index].open, &stocks[index].high,
                          &stocks[index].low, &stocks[index].close, &stocks[index].volume) != EOF) {
                index++;
            }

            fclose(file);

            // 가장 마지막 인덱스 계산
            int last_index = index - 1; // 가장 마지막 인덱스를 계산

            // 주식 데이터 분석
            analyze_stock(&stocks[last_index]); // 가장 마지막 인덱스 데이터 분석

            // 시스템 정보 출력
            printf("System Info: \n");
            system("uname -a");

            // exec 함수 사용 예제
            char *args[] = {"ls", "-l", NULL};
            execvp(args[0], args);

            // 자식 프로세스 종료
            exit(0);
        } else {
            // 부모 프로세스: 자식 프로세스 종료 대기
            wait(NULL);
            printf("Child process finished.\n");
        }
    }

    return 0;
}

// 주식 데이터 분석 함수
void analyze_stock(struct stock_data* stock) {
    printf("Analyzing stock: %s\n", stock->symbol);
    // 간단한 분석 예제
    float avg = (stock->high + stock->low) / 2;
    printf("Average price for %s: %.2f\n", stock->symbol, avg);
}
