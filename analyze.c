#include <stdio.h>

// 주식 데이터 구조체 선언
struct stock_data {
    char symbol[10];
    float open;
    float high;
    float low;
    float close;
    int volume;
};

// 주식 데이터를 분석하는 함수
void analyze_stock(struct stock_data* stock) {
    printf("Analyzing stock: %s\n", stock->symbol);
    // 주식 데이터의 고가와 저가의 평균을 계산
    float avg = (stock->high + stock->low) / 2;
    printf("Average price for %s: %.2f\n", stock->symbol, avg);
}
