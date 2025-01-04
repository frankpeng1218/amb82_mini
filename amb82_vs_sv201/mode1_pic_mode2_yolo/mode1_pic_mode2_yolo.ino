#include "VideoStream.h"
#include "AmebaFatFS.h"
#include "NNObjectDetection.h"
#include "StreamIO.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"
#include "PowerMode.h"

// General configurations
#define CHANNEL 0
#define FILENAME_PREFIX "image_"
#define NUM_IMAGES 10

// YOLO configurations
#define CHANNELNN 3
#define NNWIDTH  576
#define NNHEIGHT 320

// Mode selection (1 for Capture and Save Images, 2 for YOLO Processing)
#define MODE 2

// Trigger configurations
#define TRIGGER_PIN 2
bool TRIGGER_FLAG = true;
float TRIGGER_DELAY_TIME = 0; // in seconds

// Power mode configuration
#define WAKEUP_SOURCE 0  // AON Timer
#if (WAKEUP_SOURCE == 0)
    #define CLOCK 0
    #define SLEEP_DURATION 20
    uint32_t PM_AONtimer_setting[2] = {CLOCK, SLEEP_DURATION};
    #define WAKEUP_SETTING (uint32_t)(PM_AONtimer_setting)
#endif

// Video settings
VideoSetting config(VIDEO_VGA, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 5, VIDEO_RGB, 0); // 初始化 YOLO 模式使用的设置

// Objects for camera, SD card, and YOLO
AmebaFatFS fs;
NNObjectDetection ObjDet;      // YOLO 对象
StreamIO videoStreamerNN(1, 1); // 视频流对象

// 全域變數
int cycle = 1;

// Time measurement variables
unsigned long preparation_time; // From power-on to ready for capture
unsigned long start_time;       // Time when capture starts
unsigned long end_time;         // Time when process ends

// Function to save `cycle` value
void saveCycleValue() {
    File file = fs.open("/cycle.txt"); // 打開檔案
    if (file) {
        file.seek(0);                  // 從檔案開頭寫入
        file.print(cycle);             // 將 cycle 的值寫入檔案
        file.close();
        Serial.println("Cycle value saved.");
    } else {
        Serial.println("Failed to save cycle value.");
    }
}

// Function to load `cycle` value
void loadCycleValue() {
    File file = fs.open("/cycle.txt"); // 打開檔案
    if (file) {
        String value = file.readString(); // 讀取檔案內容
        cycle = value.toInt();            // 轉換為整數
        file.close();
        Serial.print("Cycle value loaded: ");
        Serial.println(cycle);
    } else {
        Serial.println("No saved cycle value found. Starting at 1.");
        cycle = 1; // 如果無檔案，初始化為 1
    }
}

// Function to enter Standby mode
void enterStandby() {
    saveCycleValue(); // 保存 cycle 的值
    Serial.println("Entering Standby Mode...");
    PowerMode.begin(STANDBY_MODE, WAKEUP_SOURCE, WAKEUP_SETTING);
    PowerMode.start();
    while (1);
}

void captureAndSaveImages() {
    uint32_t img_addr = 0;
    uint32_t img_len = 0;

    start_time = millis();

    for (int i = 0; i < NUM_IMAGES; i++) {
        String file_name = String(fs.getRootPath()) + String(FILENAME_PREFIX) + String(cycle) + "_" + String(i) + ".jpg";

        File file = fs.open(file_name);
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        file.write((uint8_t*)img_addr, img_len);
        file.close();

        Serial.print("Image ");
        Serial.print(i);
        Serial.println(" saved.");
    }

    end_time = millis();

    Serial.print("Preparation before photo: ");
    Serial.print(preparation_time);
    Serial.println(" ms");

    Serial.print("Total execution time: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");

    cycle++;
    enterStandby();
}

void captureAndProcessWithYOLO() {
    uint32_t img_addr = 0;
    uint32_t img_len = 0;

    ObjDet.configVideo(configNN);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    start_time = millis();

    for (int frame = 1; frame <= NUM_IMAGES; frame++) {
        Camera.getImage(CHANNEL, &img_addr, &img_len);

        std::vector<ObjectDetectionResult> results = ObjDet.getResult();
        int objectCount = ObjDet.getResultCount();

        String detectionStatus = (objectCount > 0) ? "1" : "0";
        String fileName = String(fs.getRootPath()) + "frame_" + String(cycle) + "_" + detectionStatus + "_" + String(frame) + ".jpg";

        File file = fs.open(fileName);
        if (file) {
            file.write((uint8_t*)img_addr, img_len);
            file.close();
            Serial.print("Saved ");
            Serial.println(fileName);
        } else {
            Serial.println("Failed to save file");
        }

        Serial.print("Frame ");
        Serial.print(frame);
        Serial.print(": Detected ");
        Serial.print(objectCount);
        Serial.println(" objects.");
    }

    end_time = millis();

    Serial.print("Preparation before photo: ");
    Serial.print(preparation_time);
    Serial.println(" ms");

    Serial.print("Total execution time: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");

    cycle++;
    enterStandby();
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIGGER_PIN, INPUT);

    // 等待觸發
    if (TRIGGER_FLAG) {
        while (digitalRead(TRIGGER_PIN) != HIGH) {
            delay(10); // Polling interval
        }
        delay((int)(TRIGGER_DELAY_TIME * 1000)); // 加入延遲
    }

    // 初始化序列埠和 SD 卡
    fs.begin(); 
    loadCycleValue(); // 載入週期值

    unsigned long init_start = millis();

    // 攝影機初始化
    if (MODE == 1) {
        Camera.configVideoChannel(CHANNEL, config);
        Camera.videoInit();
        Camera.channelBegin(CHANNEL);
    } else if (MODE == 2) {
        Camera.configVideoChannel(CHANNEL, config);
        Camera.configVideoChannel(CHANNELNN, configNN);
        Camera.videoInit();
        Camera.channelBegin(CHANNEL);
        Camera.channelBegin(CHANNELNN);
    } else {
        Serial.println("cam initialize failed");
    }

    delay(1);
    preparation_time = millis() - init_start;

    // 執行主功能
    if (MODE == 1) {
        captureAndSaveImages();
    } else if (MODE == 2) {
        captureAndProcessWithYOLO();
    } else {
        Serial.println("Invalid MODE defined.");
    }
}

void loop() {
    while (true) {
        delay(1000); // 保持程式運行
    }
}
