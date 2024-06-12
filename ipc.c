#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

// 주식 데이터 구조체
struct stock_data {
    char symbol[10];
    float open;
    float high;
    float low;
    float close;
    int volume;
};

// 메시지 큐 구조체
struct msg_buffer {
    long msg_type;
    struct stock_data stock;
};

// 주식 데이터를 분석 함수
void analyze_stock2(struct stock_data* stock) {
    printf("Symbol: %s\n", stock->symbol);
    printf("Open: %.2f\n", stock->open);
    printf("High: %.2f\n", stock->high);
    printf("Low: %.2f\n", stock->low);
    printf("Close: %.2f\n", stock->close);
    printf("Volume: %d\n", stock->volume);
}

// CSV 파일을 읽는 함수
struct stock_data read_last_line_from_csv(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    char line[256];
    char last_line[256];
    while (fgets(line, sizeof(line), file)) {
        strcpy(last_line, line);
    }
    fclose(file);

    struct stock_data stock;
    char *last = strtok(last_line, ",");
    last = strtok(NULL, ",");
    strcpy(stock.symbol, last);
    stock.open = atof(strtok(NULL, ","));
    stock.high = atof(strtok(NULL, ","));
    stock.low = atof(strtok(NULL, ","));
    stock.close = atof(strtok(NULL, ","));
    stock.volume = atoi(strtok(NULL, ","));
    return stock;
}

// IPC 핸들링 함수
void handle_ipc() {
    int fd[2];
    if (pipe(fd) == -1) { //파이프 생성
        perror("Pipe 생성 실패");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) { // 자식 프로세스 생성
        perror("Fork 실패");
        exit(1);
    }

    if (pid == 0) { // 자식 프로세스: 파이프에서 데이터 읽기
        close(fd[1]); // 쓰기 끝 닫기
        struct stock_data received_stock;
        read(fd[0], &received_stock, sizeof(received_stock));
        close(fd[0]);

        // 파이프에서 받은 데이터 출력
        printf("파이프에서 주식 데이터를 받았습니다.:\n");
        analyze_stock2(&received_stock);

        // 공유 메모리를 사용하여 데이터 전송
        key_t key = 1234;
        int shmid = shmget(key, sizeof(struct stock_data), 0666 | IPC_CREAT);
        if (shmid < 0) {
            perror("shmget 실패");
            exit(1);
        }
        struct stock_data* shm_stock = shmat(shmid, NULL, 0);

        // 공유 메모리에서 데이터 읽기
        printf("Received stock data from shared memory:\n");
        analyze_stock2(shm_stock);

        // 공유 메모리 분리
        shmdt(shm_stock);

        // 메시지 큐에서 데이터 읽기
        int msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid < 0) {
            perror("msgget failed");
            exit(1);
        }
        struct msg_buffer msg;
        if (msgrcv(msgid, &msg, sizeof(msg.stock), 1, 0) < 0) {
            perror("msgrcv failed");
            exit(1);
        }
        printf("Received stock data from message queue:\n");
        analyze_stock2(&msg.stock);

        // 메모리 매핑을 사용하여 파일에서 데이터 읽기
        int fd_mmap = open("mapped_stock.dat", O_RDONLY);
        if (fd_mmap < 0) {
            perror("Failed to open file for reading");
            exit(1);
        }
        struct stock_data* mapped_stock = mmap(NULL, sizeof(struct stock_data), PROT_READ, MAP_SHARED, fd_mmap, 0);
        if (mapped_stock == MAP_FAILED) {
            perror("mmap failed");
            close(fd_mmap);
            exit(1);
        }
        close(fd_mmap);

        // 메모리 매핑된 데이터 읽기
        printf("Received stock data from memory-mapped file:\n");
        analyze_stock2(mapped_stock);

        // 메모리 매핑 해제
        munmap(mapped_stock, sizeof(struct stock_data));

    } else {
        // 부모 프로세스: CSV 파일 읽기
        struct stock_data parent_stock = read_last_line_from_csv("nasdaq_stocks_6months.csv");

        // 파이프에 데이터 쓰기
        close(fd[0]); // 읽기 끝 닫기
        write(fd[1], &parent_stock, sizeof(parent_stock));
        close(fd[1]);

        // 공유 메모리를 사용하여 데이터 전송
        key_t key = 1234;
        int shmid = shmget(key, sizeof(struct stock_data), 0666 | IPC_CREAT);
        if (shmid < 0) {
            perror("shmget failed");
            exit(1);
        }
        struct stock_data* shm_stock = shmat(shmid, NULL, 0);
        if (shm_stock == (void*)-1) {
            perror("shmat failed");
            exit(1);
        }

        // 공유 메모리에 데이터 쓰기
        *shm_stock = parent_stock;

        // 공유 메모리 분리
        shmdt(shm_stock);

        // 메시지 큐에 데이터 쓰기
        int msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid < 0) {
            perror("msgget failed");
            exit(1);
        }
        struct msg_buffer msg;
        msg.msg_type = 1;
        msg.stock = parent_stock;
        if (msgsnd(msgid, &msg, sizeof(msg.stock), 0) < 0) {
            perror("msgsnd failed");
            exit(1);
        }

        // 메모리 매핑을 사용하여 파일에 데이터 쓰기
        int fd_mmap = open("mapped_stock.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd_mmap < 0) {
            perror("Failed to open file for writing");
            exit(1);
        }
        if (ftruncate(fd_mmap, sizeof(struct stock_data)) == -1) {
            perror("Failed to truncate file");
            close(fd_mmap);
            exit(1);
        }
        struct stock_data* mapped_stock = mmap(NULL, sizeof(struct stock_data), PROT_WRITE, MAP_SHARED, fd_mmap, 0);
        if (mapped_stock == MAP_FAILED) {
            perror("mmap failed");
            close(fd_mmap);
            exit(1);
        }
        close(fd_mmap);

        // 메모리 매핑된 파일에 데이터 쓰기
        *mapped_stock = parent_stock;

        // 메모리 매핑 해제
        munmap(mapped_stock, sizeof(struct stock_data));

        // 자식 프로세스 종료 대기
        wait(NULL);
        printf("Parent process finished.\n");

        // 공유 메모리 삭제
        shmctl(shmid, IPC_RMID, NULL);

        // 메시지 큐 삭제
        msgctl(msgid, IPC_RMID, NULL);

        // 자식 프로세스가 종료될 때까지 대기
        int status;
        waitpid(pid, &status, 0);
    }
}

