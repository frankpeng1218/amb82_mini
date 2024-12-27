/*
 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-neuralnework-object-detection/

 This version removes Wi-Fi and RTSP and works locally.
*/

#include "StreamIO.h"
#include "VideoStream.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"

#define CHANNEL 0
#define CHANNELNN 3 

// Lower resolution for NN processing
#define NNWIDTH  576
#define NNHEIGHT 320

VideoSetting config(VIDEO_FHD, 5, VIDEO_H264, 0);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 5, VIDEO_RGB, 0);
NNObjectDetection ObjDet;
StreamIO videoStreamerNN(1, 1);

void setup() {
    Serial.begin(115200);

    // Configure camera video channels with video format information
    config.setBitrate(2 * 1024 * 1024); // Adjust the bitrate as needed
    Camera.configVideoChannel(CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.videoInit();

    // Configure object detection with corresponding video format information
    ObjDet.configVideo(configNN);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    // Configure StreamIO object to stream data from RGB video channel to object detection
    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.setStackSize();
    videoStreamerNN.setTaskPriority();
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Start video channels
    Camera.channelBegin(CHANNEL);
    Camera.channelBegin(CHANNELNN);

    // Start OSD drawing on video channel
    OSD.configVideo(CHANNEL, config);
    OSD.begin();
}

void loop() {
    // Measure the time taken for YOLO inference
    std::vector<ObjectDetectionResult> results = ObjDet.getResult();

    printf("Total number of objects detected = %d\r\n", ObjDet.getResultCount());

    if (ObjDet.getResultCount() > 0) {
        for (uint32_t i = 0; i < ObjDet.getResultCount(); i++) {
            int obj_type = results[i].type();
            if (itemList[obj_type].filter) {    // check if item should be ignored

                ObjectDetectionResult item = results[i];
                // Print identification text
                printf("Item %d %s: Confidence = %d\n\r", i, itemList[obj_type].objectName, item.score());
            }
        }
    }

    // delay to wait for new results
    delay(500);
}
