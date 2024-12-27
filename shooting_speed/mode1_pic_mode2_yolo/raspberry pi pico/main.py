from INA219 import INA219
import machine
import utime

# 設定 I2C 通訊
sda = machine.Pin(16)
scl = machine.Pin(17)
i2c = machine.I2C(0, sda=sda, scl=scl, freq=400000)

# 初始化 INA219 傳感器
currentSensor = INA219(i2c)
currentSensor.set_calibration_16V_400mA()

# 取樣速率
sampling_rate = 300


while True:
    bus_voltage = currentSensor.bus_voltage
    current = currentSensor.current
    power = current * bus_voltage

    # 傳送簡化數據，避免解析延遲
    print(f"{bus_voltage:.2f},{current:.2f},{power:.2f}")
    utime.sleep_us(int(1e6 / sampling_rate))
