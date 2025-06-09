#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define PH_SENSOR_PIN A0
#define NITRATE_SENSOR_PIN A1
#define BATTERY_MONITOR_PIN A2
#define BUTTON_PIN 2
#define SD_CS_PIN 10

#define MEASUREMENT_INTERVAL 900000UL

void manualInterrupt();
void setupRTC();
void setupSleep();
void enterSleep();
void takeReading();
void logError(String message);
void createFileIfNotExists(String filename);
void writeData(String data);
String getDateString();
String getTimeString();
float readBatteryVoltage();

void setup() {
    Serial.begin(9600);

    // input output pins
    pinMode(PH_SENSOR_PIN, INPUT);
    pinMode(NITRATE_SENSOR_PIN, INPUT);
    pinMode(BATTERY_MONITOR_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(SD_CS_PIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), manualInterrupt, FALLING);

    if (!SD.begin(SD_CS_PIN)) {
        logError("SD Card init fail");
        while (1);
    }

    setupRTC();
    setupSleep();

    createFileIfNotExists(getDateString() + ".csv");
}

void loop() {
    takeReading();
}