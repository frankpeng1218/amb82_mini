#include "VideoStream.h"
#include "AmebaFatFS.h"
#include "NNObjectDetection.h"
#include "StreamIO.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"

// Camera and NN configurations
#define CHANNEL 0
#define CHANNELNN 3
#define NNWIDTH  576
#define NNHEIGHT 320

// Number of frames to process
#define NUM_FRAMES 5

VideoSetting config(VIDEO_HD, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 5, VIDEO_RGB, 0);
NNObjectDetection ObjDet;
StreamIO videoStreamerNN(1, 1);

AmebaFatFS fs;
uint32_t img_addr = 0;
uint32_t img_len = 0;

// Time measurement variables
unsigned long start_time = 0;
unsigned long end_time = 0;

void setup() {
    Serial.begin(115200);

    // Record start time
    start_time = millis();

    // Initialize camera
    Camera.configVideoChannel(CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);
    Camera.channelBegin(CHANNELNN);

    // Initialize SD card
    fs.begin();

    // Initialize object detection
    ObjDet.configVideo(configNN);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    // Configure StreamIO for YOLO
    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }
}

void processFrame(int frameIndex) {
    // Capture frame
    Camera.getImage(CHANNEL, &img_addr, &img_len);

    // Perform YOLO object detection
    std::vector<ObjectDetectionResult> results = ObjDet.getResult();
    int objectCount = ObjDet.getResultCount();

    // Generate file name
    String fileName = String(fs.getRootPath()) + "frame_" + String(frameIndex + 1) + "_" + String(objectCount) + ".jpg";

    // Save image to SD card
    File file = fs.open(fileName);
    if (file) {
        file.write((uint8_t*)img_addr, img_len);
        file.close();
        Serial.print("Saved ");
        Serial.println(fileName);
    } else {
        Serial.println("Failed to save file");
    }

    // Print detection results
    Serial.print("Frame ");
    Serial.print(frameIndex + 1);
    Serial.print(": Detected ");
    Serial.print(objectCount);
    Serial.println(" objects.");
    if (objectCount > 0) {
        for (size_t i = 0; i < results.size(); i++) {
            int objType = results[i].type();
            if (itemList[objType].filter) {
                ObjectDetectionResult item = results[i];
                Serial.print(" - ");
                Serial.print(itemList[objType].objectName);
                Serial.print(": Confidence = ");
                Serial.println(item.score());
            }
        }
    }
}

void loop() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        processFrame(i);
    }

    // Record end time
    end_time = millis();

    // Calculate and print total elapsed time
    unsigned long elapsed_time = end_time - start_time;
    Serial.print("Total execution time: ");
    Serial.print(elapsed_time);
    Serial.println(" ms");

    // Finalize SD card operations
    fs.end();

    // Stop further execution
    while (true) {
        // delay(1000);
    }
}
