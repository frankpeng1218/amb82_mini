import serial
import time
import threading

# 設定 Serial
ser = serial.Serial('/dev/tty.usbmodem13101', baudrate=921600, timeout=1)

# 設定全域變數
sample_count = 0

# 串口數據讀取
def read_serial():
    global sample_count
    while True:
        try:
            raw = ser.readline().decode().strip()
            if raw:
                sample_count += 1  # 記錄接收到的樣本數
        except Exception as e:
            print(f"Error reading serial: {e}")
            continue

# 啟動讀取數據執行緒
thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# 每秒統計一次接收到的樣本數
while True:
    start_time = time.time()
    start_sample_count = sample_count
    time.sleep(1)  # 等待一秒
    end_time = time.time()
    end_sample_count = sample_count

    # 計算實際接收速率
    actual_sample_rate = (end_sample_count - start_sample_count) / (end_time - start_time)
    print(f"實際接收速率: {actual_sample_rate:.2f} samples/sec")
