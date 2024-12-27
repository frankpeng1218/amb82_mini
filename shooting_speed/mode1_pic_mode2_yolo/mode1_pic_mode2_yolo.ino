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
#define NUM_IMAGES 3

// YOLO configurations
#define CHANNELNN 3
#define NNWIDTH  576
#define NNHEIGHT 320

// Mode selection (1 for Capture and Save Images, 2 for YOLO Processing)
#define MODE 2

// Power mode configuration
#define WAKEUP_SOURCE 0  // AON Timer
#if (WAKEUP_SOURCE == 0)
    #define CLOCK 0
    #define SLEEP_DURATION 20
    uint32_t PM_AONtimer_setting[2] = {CLOCK, SLEEP_DURATION};
    #define WAKEUP_SETTING (uint32_t)(PM_AONtimer_setting)
#endif

// Video settings
VideoSetting config(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 5, VIDEO_RGB, 0); // 初始化 YOLO 模式使用的设置

// Objects for camera, SD card, and YOLO
AmebaFatFS fs;
NNObjectDetection ObjDet;      // YOLO 对象
StreamIO videoStreamerNN(1, 1); // 视频流对象

// Time measurement variables
unsigned long preparation_time; // From power-on to ready for capture
unsigned long start_time;       // Time when capture starts
unsigned long end_time;         // Time when process ends

// Function to enter Standby mode
void enterStandby() {
    Serial.println("Entering Standby Mode...");
    PowerMode.begin(STANDBY_MODE, WAKEUP_SOURCE, WAKEUP_SETTING);
    PowerMode.start();
    Serial.println("You won't see this log after entering Standby Mode");
    while (1); // Ensure no further code execution
}

void captureAndSaveImages() {
    uint32_t img_addr = 0;
    uint32_t img_len = 0;

    // Record the time capture starts
    start_time = millis();

    for (int i = 0; i < NUM_IMAGES; i++) {
        // Prepare file name
        String file_name = String(fs.getRootPath()) + String(FILENAME_PREFIX) + String(i) + ".jpg";

        // Open file
        File file = fs.open(file_name);

        // Capture image
        Camera.getImage(CHANNEL, &img_addr, &img_len);

        // Save image
        file.write((uint8_t*)img_addr, img_len);
        file.close();

        Serial.print("Image ");
        Serial.print(i);
        Serial.println(" saved.");
    }

    // Record end time
    end_time = millis();

    Serial.print("Preparation before photo: ");
    Serial.print(preparation_time);
    Serial.println(" ms");

    Serial.print("Total execution time: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");

    enterStandby();
}

void captureAndProcessWithYOLO() {
    uint32_t img_addr = 0;
    uint32_t img_len = 0;

    // Initialize object detection (YOLO)
    ObjDet.configVideo(configNN);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Record the time capture starts
    start_time = millis();

    for (int i = 0; i < NUM_IMAGES; i++) {
        // Capture frame
        Camera.getImage(CHANNEL, &img_addr, &img_len);

        // Perform YOLO object detection
        std::vector<ObjectDetectionResult> results = ObjDet.getResult();
        int objectCount = ObjDet.getResultCount();

        // Generate file name
        String fileName = String(fs.getRootPath()) + "frame_" + String(i + 1) + "_" + String(objectCount) + ".jpg";

        // Save image
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
        Serial.print(i + 1);
        Serial.print(": Detected ");
        Serial.print(objectCount);
        Serial.println(" objects.");
    }

    // Record end time
    end_time = millis();

    Serial.print("Preparation before photo: ");
    Serial.print(preparation_time);
    Serial.println(" ms");

    Serial.print("Total execution time: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");

    enterStandby();
}

void setup() {
    Serial.begin(115200);

    // Record the time from power-on to ready
    unsigned long init_start = millis();

    if (MODE == 1) {
        // Initialize camera
        Camera.configVideoChannel(CHANNEL, config);
        Camera.videoInit();
        Camera.channelBegin(CHANNEL);
    } else if (MODE == 2) {
        // Initialize camera
        Camera.configVideoChannel(CHANNEL, config);
        Camera.configVideoChannel(CHANNELNN, configNN);
        Camera.videoInit();
        Camera.channelBegin(CHANNEL);
        Camera.channelBegin(CHANNELNN);
    } else {
        Serial.println("cam initualize failed");
    }


    // Initialize SD card
    fs.begin();

    // Add delay to ensure proper hardware initialization
    delay(1); 

    // Record preparation time
    preparation_time = millis() - init_start;

    // Execute based on mode
    if (MODE == 1) {
        captureAndSaveImages();
    } else if (MODE == 2) {
        captureAndProcessWithYOLO();
    } else {
        Serial.println("Invalid MODE defined.");
    }
}

void loop() {
    // Stop further execution
    while (true) {
        delay(1000);
    }
}
