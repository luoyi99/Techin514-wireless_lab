#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

// BLE Server and Characteristic declarations
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// HC-SR04 Sensor pins
const int triggerPin = 2; // Change to your trigger pin
const int echoPin = 1;   // Change to your echo pin

// DSP Variables
const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total = 0;
float average = 0;

// UUIDs for BLE service and characteristic
#define SERVICE_UUID        "014805bf-7587-47e5-8965-b11fef29d012"
#define CHARACTERISTIC_UUID "47b36193-f67b-4a77-ad27-0cafb5dd7a91"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Initialize HC-SR04 sensor
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0.0;
    }

    // BLE setup
    BLEDevice::init("XIAO_ESP32S3_HAHAHAHA");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Connected to HAHAHAHA's ESP32");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // Measure distance from HC-SR04 sensor
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)

    // Moving Average Filter
    total -= readings[readIndex];
    readings[readIndex] = distance;
    total += readings[readIndex];
    readIndex++;
    if (readIndex >= numReadings) {
        readIndex = 0;
    }
    average = total / numReadings;

    // Print raw and denoised data
    Serial.print("Raw Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Average Distance: ");
    Serial.print(average);
    Serial.println(" cm");

    // Send data over BLE if average < 30 cm
    if (deviceConnected && average < 30) {
        // String valueToSend = "Distance: " + String(average) + " cm";
        String valueToSend = String(average);
        pCharacteristic->setValue(valueToSend.c_str());
        pCharacteristic->notify();
        Serial.println("Data sent over BLE: " + valueToSend);
    }

    // Disconnecting and reconnecting handling
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // Restart advertising
        oldDeviceConnected = deviceConnected;
        Serial.println("Start advertising");
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(1000); // Delay to make it more readable
}
