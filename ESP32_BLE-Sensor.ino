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

BLECharacteristic *pCharacteristicBatt;
BLECharacteristic *pCharacteristicElev;
bool deviceConnected = false;

uint8_t batterylevel = 100;
int elevation = 0;  // 24 bit

// Define Service UUIDs:
#define SERVICE_BATTERY         (uint16_t)0x180F  // Name: Battery Service
#define SERVICE_ENVSENS         (uint16_t)0x181A  // Name: Environmental Sensing

// Define Characteristic UUIDs:
#define CHARACTERISTIC_BATTERY  (uint16_t)0x2A19  // Name: Battery Level
#define CHARACTERISTIC_ELEVATION (uint16_t)0x2A6C  // Name: Elevation


// BLE Callback
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
}; // BLE Callback

// Get Elevation
uint16_t getElevation() {
  return random(0x000, 0xFFFFFF);
} // Get Elevation

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
  BLEService *pServiceElev = pServer->createService(SERVICE_ENVSENS);

  // Create a BLE Characteristic for Batterylevel
  pCharacteristicBatt = pServiceBatt->createCharacteristic(
                          BLEUUID(CHARACTERISTIC_BATTERY),
                          BLECharacteristic::PROPERTY_READ   |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

  // Create a BLE Characteristic for Altitude
  pCharacteristicElev = pServiceElev->createCharacteristic(
                          BLEUUID(CHARACTERISTIC_ELEVATION),
                          BLECharacteristic::PROPERTY_READ   |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristicBatt->addDescriptor(new BLE2902());
  pCharacteristicElev->addDescriptor(new BLE2902());

  // Start the service
  pServiceBatt->start();
  pServiceElev->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
} // Setup

// Main
void loop() {
  elevation = getElevation();

  if (batterylevel > 5) {
    batterylevel = batterylevel - 5;
  } else {
    batterylevel = 100;
  }

  if (deviceConnected) {
    pCharacteristicBatt->setValue((uint8_t*)&batterylevel, 1);
    pCharacteristicBatt->notify();

    Serial.printf("*** NOTIFY: %d ***\n", elevation);
    //the function excepts a uint8_t pointer - we point to a int16_t so the size is 2
    pCharacteristicElev->setValue((uint8_t*)&elevation, 3);
    pCharacteristicElev->notify();
  }
  delay(2000);
} // Main

