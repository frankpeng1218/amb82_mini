#include "VideoStream.h"
#include "AmebaFatFS.h"
#include "PowerMode.h"

#define CHANNEL 0
#define FILENAME "image.jpg"

VideoSetting config(VIDEO_VGA, CAM_FPS, VIDEO_JPEG, 1);

uint32_t img_addr = 0;
uint32_t img_len = 0;

AmebaFatFS fs;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Camera Capture and Timing...");

    // 初始化相機
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);

    // 記錄拍照開始時間
    unsigned long start_time = millis();

    // 拍照
    Camera.getImage(CHANNEL, &img_addr, &img_len);


    unsigned long end_time = millis();

    unsigned long capture_time = end_time - start_time;

    fs.begin();

    File file = fs.open(String(fs.getRootPath()) + String(FILENAME));
    if (file) {
        file.write((uint8_t*)img_addr, img_len);
        file.close();
        Serial.println("Image saved to SD card.");
    } else {
        Serial.println("Failed to open file on SD card.");
    }

    fs.end();

    Serial.print("Image capture time (excluding save): ");
    Serial.print(capture_time);
    Serial.println(" ms");

    PowerMode.begin(DEEPSLEEP_MODE, 0, 0);
    PowerMode.start();
}

void loop() {
}
