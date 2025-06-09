// library imports
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <avr/sleep.h>
#include <avr/power.h>

// pin definitions
#define PH_SENSOR_PIN A0
#define NITRATE_SENSOR_PIN A1
#define BATTERY_MONITOR_PIN A2
#define BUTTON_PIN 2
#define SD_CS_PIN 10

// timing interval
// i've chosen 15 minutes
#define MEASUREMENT_INTERVAL 900000UL

// calibration
const float PH_SLOPE = -3.0;
const float PH_INTERCEPT = 14.0;
const float NITRATE_SLOPE = 100.0;
const float NITRATE_INTERCEPT = 0.0;

// global
RTC_DS3231 rtc;
volatile bool takeManualReading = false;
unsigned long lastInterruptTime = 0;
const float VOLTAGE_DIVIDER_RATIO = 4.0;

// function declarations
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

// setup
void setup() {
    Serial.begin(9600);

    // input output pins
    pinMode(PH_SENSOR_PIN, INPUT);
    pinMode(NITRATE_SENSOR_PIN, INPUT);
    pinMode(BATTERY_MONITOR_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(SD_CS_PIN, OUTPUT);

    // manual button
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), manualInterrupt, FALLING);

    // init SD card
    if (!SD.begin(SD_CS_PIN)) {
        logError("SD Card init fail");
        // if fails
        while (1);
    }

    // init clock
    setupRTC();
    // init sleep
    setupSleep();

    createFileIfNotExists(getDateString() + ".csv");
}

void loop() {
    takeReading();

    // at interval / reading req.
    unsigned long startSleep = millis();

    while (millis() - startSleep < MEASUREMENT_INTERVAL) {
        if (takeManualReading) {
            takeManualReading = false;
            takeReading();
        } else {
            enterSleep();
        }
    }
}

// for debouncing
void manualInterrupt() {
    unsigned long currentTime = millis();

    if (currentTime - lastInterruptTime > 200) {
        takeManualReading = true;
        lastInterruptTime = currentTime;
    }
}

void setupRTC() {
    if (!rtc.begin()) {
        logError("RTC not found");
        // if fails
        while (1);
    }

    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        logError("RTC time reset");
    }
}

void setupSleep() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
}

void enterSleep() {
    sleep_enable();
    noInterrupts();
    sleep_bod_disable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

void takeReading() {
    DateTime now = rtc.now();

    float phVoltage = analogRead(PH_SENSOR_PIN) * (5.0 / 1023.0);
    float nitrateVoltage = analogRead(NITRATE_SENSOR_PIN) * (5.0 / 1023.0);
    float phValue = PH_SLOPE * phVoltage + PH_INTERCEPT;
    float nitrateValue = NITRATE_SLOPE * nitrateVoltage + NITRATE_INTERCEPT;

    float batteryVoltage = readBatteryVoltage();

    String timeStamp = getTimeString();
    String dataString = timeStamp + "," + String(phValue, 2) + "," + String(nitrateValue, 2) + "," + String(batteryVoltage, 2);

    writeData(dataString);

    // if battery too low
    if (batteryVoltage < 3.3) {
        logError("low battery: " + String(batteryVoltage, 2) + "V");
    }
}

void logError(String message) {
    File errorFile = SD.open("error.log", FILE_WRITE);

    if (errorFile) {
        DateTime now = rtc.now();
        errorFile.print(now.timestamp(DateTime::TIMESTAMP_FULL));
        errorFile.print(", ");
        errorFile.println(message);
        errorFile.close();
    }
}

void createFileIfNotExists(String filename) {
    if (!SD.exists(filename)) {
        File f = SD.open(filename, FILE_WRITE);
        if (f) {
            f.println("Time,PH,Nitrate,BatteryV");
            f.close();
        } else {
            logError("create file fail");
        }
    }
}

void writeData(String data) {
    String filename = getDateString() + ".csv";
    File dataFile = SD.open(filename, FILE_WRITE);

    if (dataFile) {
        dataFile.println(data);
        dataFile.close();
    } else {
        logError("fail to write data");
    }
}

String getDateString() {
    DateTime now = rtc.now();
    char dateStr[11];
    sprintf(dateStr, "%02d-%02d-%04d", now.day(), now.month(), now.year());

    return String(dateStr);
}

String getTimeString() {
    DateTime now = rtc.now();
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    return String(timeStr);
}

float readBatteryVoltage() {
    int sensorValue = analogRead(BATTERY_MONITOR_PIN);
    float voltage = sensorValue * (5.0 / 1023.0);

    return voltage * VOLTAGE_DIVIDER_RATIO;
}
