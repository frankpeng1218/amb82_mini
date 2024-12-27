/*  This example uses the camera to capture multiple JPEG images,
    and saves them to SD Card while measuring compression and save times.

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-video-jpeg-sdcard/
*/
#include "VideoStream.h"
#include "AmebaFatFS.h"

#define CHANNEL 0
#define FILENAME_PREFIX "image_"
#define NUM_IMAGES 5

VideoSetting config(VIDEO_HD, CAM_FPS, VIDEO_JPEG, 1);

uint32_t img_addr = 0;
uint32_t img_len = 0;

AmebaFatFS fs;

unsigned long start_time;
unsigned long pre_capture_time;
unsigned long compression_start;
unsigned long compression_end;
unsigned long save_start;
unsigned long save_end;

void setup() {
    Serial.begin(115200);

    // Record the start time of the program
    start_time = millis();

    // Initialize Camera
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);

    // Record the time taken to prepare before capturing images
    pre_capture_time = millis();

    // Initialize SD Card
    fs.begin();

    for (int i = 0; i < NUM_IMAGES; i++) {
        // Prepare file name
        String file_name = String(fs.getRootPath()) + String(FILENAME_PREFIX) + String(i) + ".jpg";

        // Open file
        File file = fs.open(file_name);

        // Capture image and measure compression time
        compression_start = millis();
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        compression_end = millis();

        // Save image and measure save time
        save_start = millis();
        file.write((uint8_t*)img_addr, img_len);
        save_end = millis();

        file.close();

        // Output timing details for each image
        Serial.print("Image ");
        Serial.print(i);
        Serial.print(": Compression time = ");
        Serial.print(compression_end - compression_start);
        Serial.print(" ms, Save time = ");
        Serial.print(save_end - save_start);
        Serial.println(" ms");
    }

    // Finalize SD Card
    fs.end();

    // Output time taken before capturing images
    Serial.print("Time before first capture = ");
    Serial.print(pre_capture_time - start_time);
    Serial.println(" ms");

    // Output total elapsed time
    Serial.print("Total execution time = ");
    Serial.print(millis() - start_time);
    Serial.println(" ms");
}

void loop() {
    // No operation in loop
    // delay(1000);
}
