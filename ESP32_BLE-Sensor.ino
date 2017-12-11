/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristicAlti;
BLECharacteristic *pCharacteristicBatt;
bool deviceConnected = false;

uint8_t batterylevel = 100;
uint16_t altitude = 0;

// Define Service UUIDs:
#define SERVICE_BATTERY         (uint16_t)0x180F  // Name: Battery Service
#define SERVICE_LOCNAV          (uint16_t)0x1819  // Name: Location and Navigation

// Define Characteristic UUIDs:
#define CHARACTERISTIC_BATTERY  (uint16_t)0x2A19  // Name: Battery Level
#define CHARACTERISTIC_ALTITUDE (uint16_t)0x2AB3  // Name: Altitude


// BLE Callback
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
}; // BLE Callback

// Get Altitude
uint16_t getAltitude() {
  return random(0x000, 0xFFFF);
} // Get Altitude

// Setup
void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("MyESP32");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service for Battery
  BLEService *pServiceBatt = pServer->createService(SERVICE_BATTERY);

  // Create the BLE Service for Location and Navigation
  BLEService *pServiceAlti = pServer->createService(SERVICE_LOCNAV);

  // Create a BLE Characteristic for Batterylevel
  pCharacteristicBatt = pServiceBatt->createCharacteristic(
                      BLEUUID(CHARACTERISTIC_BATTERY),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a BLE Characteristic for Altitude
  pCharacteristicAlti = pServiceAlti->createCharacteristic(
                      BLEUUID(CHARACTERISTIC_ALTITUDE),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristicBatt->addDescriptor(new BLE2902());
  pCharacteristicAlti->addDescriptor(new BLE2902());

  // Start the service
  pServiceBatt->start();
  pServiceAlti->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
} // Setup

// Main
void loop() {
  altitude = getAltitude();

  if (batterylevel > 5) {
    batterylevel = batterylevel - 5;
  }else {
    batterylevel = 100;
  }

  if (deviceConnected) {
    pCharacteristicBatt->setValue((uint8_t*)&batterylevel, 1);
    pCharacteristicBatt->notify();
    
    Serial.printf("*** NOTIFY: %d ***\n", altitude);
    //the function excepts a uint8_t pointer - we point to a int16_t so the size is 2
    pCharacteristicAlti->setValue((uint8_t*)&altitude, 2);
    pCharacteristicAlti->notify();
    //pCharacteristicAlti->indicate();
  }
  delay(2000);
} // Main

