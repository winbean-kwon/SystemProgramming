# Makefile

# 컴파일러 설정
CC = gcc

# 대상 실행 파일
TARGET = main

# 소스 파일
SRCS = main.c analyze.c ipc.c

# 오브젝트 파일
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 클린업
clean:
	rm -f $(TARGET) $(OBJS)

# 실행
run: all
	./$(TARGET)
