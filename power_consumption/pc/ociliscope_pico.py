import serial
import matplotlib.pyplot as plt
from collections import deque
import numpy as np
import threading

# 設定 Serial
ser = serial.Serial('/dev/tty.usbmodem13101', baudrate=921600, timeout=1)

# 設定繪圖
show_duration_flag = False  # 如果為 False，屏蔽紅色文字
window_size = 10000  # 每次顯示 10000 個樣本
average_sample_size = 1000
sample_rate = 200  # 每秒 200 個樣本
bus_voltage_data = deque([0] * window_size, maxlen=window_size)
current_data = deque([0] * window_size, maxlen=window_size)
power_data = deque([0] * window_size, maxlen=window_size)

plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 6))

# 設定子圖
line1, = ax1.plot(range(window_size), bus_voltage_data, label="Bus Voltage (V)")
ax1.set_xlim(0, window_size)
ax1.set_ylabel("Voltage (V)")
ax1.legend(loc="upper right", bbox_to_anchor=(1, 1))
ax1.grid(True, linestyle='--', linewidth=0.5)

line2, = ax2.plot(range(window_size), current_data, label="Current (mA)")
ax2.set_xlim(0, window_size)
ax2.set_ylabel("Current (mA)")
ax2.legend(loc="upper right", bbox_to_anchor=(1, 1))
ax2.grid(True, linestyle='--', linewidth=0.5)

line3, = ax3.plot(range(window_size), power_data, label="Power (mW)")
ax3.set_xlim(0, window_size)
ax3.set_ylabel("Power (mW)")
ax3.set_xlabel("Samples")  # 在 x 軸添加名稱
ax3.legend(loc="upper right", bbox_to_anchor=(1, 1))
ax3.grid(True, linestyle='--', linewidth=0.5)

# 動態文字顯示
voltage_text = ax1.text(1.02, 0.5, "", fontsize=10, color="blue", ha="left", transform=ax1.transAxes)
current_text = ax2.text(1.02, 0.5, "", fontsize=10, color="blue", ha="left", transform=ax2.transAxes)
power_text = ax3.text(1.02, 0.5, "", fontsize=10, color="blue", ha="left", transform=ax3.transAxes)

# Duration 和 Avg in Range 顯示在右下角
summary_text = fig.text(0.98, 0.02, "", fontsize=10, color="red", ha="right")

# 記錄開始和結束樣本數
start_sample = None
end_sample = None
in_peak = False  # 標記是否在高峰內
lock = threading.Lock()  # 用於執行緒同步

# 高峰區間數據記錄
peak_voltage_samples = []
peak_current_samples = []
peak_power_samples = []

# 從串口讀取數據
def read_serial():
    global start_sample, end_sample, in_peak
    global peak_voltage_samples, peak_current_samples, peak_power_samples
    current_sample = 0
    while True:
        try:
            raw = ser.readline().decode().strip()
            if raw:
                data = raw.split(',')
                bus_voltage = float(data[0])
                current = float(data[1])
                power = float(data[2])

                # 更新數據到隊列
                bus_voltage_data.append(bus_voltage)
                current_data.append(current)
                power_data.append(power)

                # 動作捕捉邏輯
                with lock:
                    if current > 15 and not in_peak:  # 當電流進入高峰
                        start_sample = current_sample
                        in_peak = True
                        # 清空高峰區間的數據
                        peak_voltage_samples.clear()
                        peak_current_samples.clear()
                        peak_power_samples.clear()
                    elif current < 15 and in_peak:  # 當電流恢復穩定
                        end_sample = current_sample
                        in_peak = False

                    # 如果在高峰內，記錄樣本數據
                    if in_peak:
                        peak_voltage_samples.append(bus_voltage)
                        peak_current_samples.append(current)
                        peak_power_samples.append(power)

                current_sample += 1
        except Exception as e:
            print(f"Error reading serial: {e}")
            continue

# 啟動讀取數據執行緒
thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# 初始化 Summary Text，在右下角
summary_text = fig.text(0.98, 0.02, "", fontsize=10, color="red", ha="right")

# 繪圖更新
while True:
    try:
        line1.set_ydata(bus_voltage_data)
        line2.set_ydata(current_data)
        line3.set_ydata(power_data)

        # 計算最近 1000 個樣本的平均值
        avg_voltage = np.mean(list(bus_voltage_data)[-average_sample_size:])
        avg_current = np.mean(list(current_data)[-average_sample_size:])
        avg_power = np.mean(list(power_data)[-average_sample_size:])

        # 更新平均值到圖表外
        voltage_text.set_text(f"Avg Voltage:\n{avg_voltage:.2f} V")
        current_text.set_text(f"Avg Current:\n{avg_current:.2f} mA")
        power_text.set_text(f"Avg Power:\n{avg_power:.2f} mW")

        # 計算動作時間並根據 flag 顯示/隱藏樣本範圍內的平均值
        with lock:
            if show_duration_flag and start_sample is not None and end_sample is not None and end_sample > start_sample:
                duration_samples = end_sample - start_sample
                duration_time = duration_samples / sample_rate  # 換算成秒

                # 計算區間內的平均值
                avg_range_voltage = np.mean(peak_voltage_samples) if peak_voltage_samples else 0
                avg_range_current = np.mean(peak_current_samples) if peak_current_samples else 0
                avg_range_power = np.mean(peak_power_samples) if peak_power_samples else 0

                # 更新 Summary Text 的內容，包括樣本數
                summary_text.set_text(f"Duration: {duration_time:.2f}s\n"
                                      f"Samples: {duration_samples}\n"
                                      f"Avg Voltage: {avg_range_voltage:.2f} V\n"
                                      f"Avg Current: {avg_range_current:.2f} mA\n"
                                      f"Avg Power: {avg_range_power:.2f} mW")
            else:
                # 隱藏 Duration 的紅色文字
                summary_text.set_text("")

        # 動態調整 y 軸範圍
        ax1.set_ylim(0, max(bus_voltage_data) * 1.1)
        ax2.set_ylim(0, max(current_data) * 1.1)
        ax3.set_ylim(0, max(power_data) * 1.1)

        plt.draw()
        plt.pause(0.001)
    except KeyboardInterrupt:
        print("Stopping...")
        break

ser.close()
