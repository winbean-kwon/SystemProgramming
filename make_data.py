import pandas as pd
import numpy as np
from datetime import datetime, timedelta

# 종목 리스트
symbols = ["AAPL", "GOOGL", "MSFT", "AMZN", "TSLA"]

try:
    # 기존 CSV 파일 읽기
    df_existing = pd.read_csv("nasdaq_stocks_6months.csv")
    df_existing['Date'] = pd.to_datetime(df_existing['Date']).dt.date  # 날짜를 date 객체로 변환
    last_date = df_existing['Date'].max()
    start_date = last_date + timedelta(days=1)  # 마지막 날짜 다음 날부터 데이터 생성
except FileNotFoundError:
    df_existing = pd.DataFrame()
    start_date = datetime.now().date() - timedelta(days=180)  # 시작 날짜 설정

end_date = datetime.now().date()  # 종료 날짜를 오늘 날짜로 설정

# 새로운 데이터가 필요한 날짜 생성
dates = pd.date_range(start_date, end_date, freq='B')  # 'B'는 영업일(평일)

# 주가 데이터 생성 함수
def generate_stock_data(symbol, dates):
    data = []
    price = np.random.uniform(100, 1500)  # 초기 가격 설정
    for date in dates:
        open_price = price
        high_price = open_price * np.random.uniform(1.01, 1.05)
        low_price = open_price * np.random.uniform(0.95, 0.99)
        close_price = np.random.uniform(low_price, high_price)
        volume = np.random.randint(1000000, 5000000)
        data.append([date.date(), symbol, open_price, high_price, low_price, close_price, volume])
        price = close_price  # 다음 날의 시작 가격은 오늘의 종가로 설정
    return data

# 모든 종목에 대해 데이터 생성
all_data = []
for symbol in symbols:
    all_data.extend(generate_stock_data(symbol, dates))

# 데이터프레임 생성
df_new = pd.DataFrame(all_data, columns=["Date", "Symbol", "Open", "High", "Low", "Close", "Volume"])

# 불필요한 NA 열 제거
df_new.dropna(axis=1, how='all', inplace=True)  # 모든 값이 NA인 열을 제거

# 기존 데이터와 새로운 데이터 합치기
if not df_existing.empty:
    # 기존 데이터에서도 NA 처리
    df_existing.dropna(axis=1, how='all', inplace=True)
    df_combined = pd.concat([df_existing, df_new], ignore_index=True)
else:
    df_combined = df_new

# CSV 파일로 저장
df_combined.to_csv("nasdaq_stocks_6months.csv", index=False)
