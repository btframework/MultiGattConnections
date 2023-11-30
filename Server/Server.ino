#include <BLEDevice.h>
#include <BLE2902.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>

#define DEVICE_NAME     "MultyGattServer"

#define SERVICE_UUID                    "a25fede7-395c-4241-adc0-481004d81900"

#define NOTIFIABLE_CHARACTERISTIC_UUID  "eabbbb91-b7e5-4e50-b7aa-ced2be8dfbbe"
#define READABLE_CHARACTERISTIC_UUID    "468dfe19-8de3-4181-b728-0902c50a5e6d"
#define WRITABLE_CHARACTERISTIC_UUID    "421754b0-e70a-42c9-90ed-4aed82fa7ac0"

#define MAX_PDU_SIZE    255


bool ClientConnected = false;
BLECharacteristic* NotifyChar = NULL;


class CServerCallback : public BLEServerCallbacks
{
public:
    virtual void onConnect(BLEServer* pServer) override
    {
        ClientConnected = true;
        Serial.println("Client connected");
    }

    virtual void onDisconnect(BLEServer* pServer) override
    {
        ClientConnected = false;

        Serial.println("Client disconnected. Restart advertising.");
        pServer->getAdvertising()->start();
    }
};


class CReadableCharacteristicCallbacks : public BLECharacteristicCallbacks
{
public:
    virtual void onRead(BLECharacteristic* pCharacteristic) override
    {
        Serial.println("Read requested");

        char* Resp = "This is simple response\0";
        pCharacteristic->setValue((uint8_t*)Resp, strlen(Resp) + 1);

        Serial.println("Response value set");
    }
};


class CWritableCharacteristicCallbacks : public BLECharacteristicCallbacks
{
public:
    // This is write only characteristic.
    virtual void onWrite(BLECharacteristic* pCharacteristic) override
    {
        Serial.println("Write requested");

        size_t Len = pCharacteristic->getLength();
        uint8_t* Data = pCharacteristic->getData();
        if (Len > 0 && Data != NULL)
            Serial.println((char*)Data);
        
        Serial.println("Write request processed");
    }
};


void setup()
{
    // Configure debug serial.
    Serial.begin(115200);
    // Wait for Serial Monitor initialization/connection.
    delay(2000);
    
    // Base BLE settings.
    Serial.println("Initialize BLE device");
    BLEDevice::init(DEVICE_NAME);
    BLEDevice::setMTU(MAX_PDU_SIZE);

    // Create server object.
    Serial.println("Create server");;
    BLEServer* Server = BLEDevice::createServer();
    // Set server callback.
    Server->setCallbacks(new CServerCallback());

    // Create message service.
    Serial.println("Create service");
    BLEService* Service = Server->createService(SERVICE_UUID);
    
    // Create notifiable characteristic.
    Serial.println("Create NOTIFIABLE characteristic");
    BLECharacteristic* Char = new BLECharacteristic(NOTIFIABLE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    Char->addDescriptor(new BLE2902());
    Service->addCharacteristic(Char);
    NotifyChar = Char;
    
    // Create readable characteristic.
    Serial.println("Create READABLED characteristic");
    Char = new BLECharacteristic(READABLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
    // Set characteristic callback.
    Char->setCallbacks(new CReadableCharacteristicCallbacks());
    Service->addCharacteristic(Char);

    // Create writable characteristic.
    Serial.println("Create WRITABLE characteristic");
    Char = new BLECharacteristic(WRITABLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    // Set characteristic callback.
    Char->setCallbacks(new CWritableCharacteristicCallbacks());
    Service->addCharacteristic(Char);
    
    // Enable service.
    Serial.println("Start server");
    Service->start();

    // Start advertising.
    Serial.println("Start advertising");
    BLEAdvertising* Advertising = BLEDevice::getAdvertising();
    Advertising->addServiceUUID(SERVICE_UUID);
    Advertising->start();

    Serial.println("GATT server ready");
}


uint32_t NotifyVal = 0;

void loop()
{
    delay(1000);
    if (ClientConnected)
    {
        Serial.print("Send notification ");
        Serial.print(NotifyVal);
        Serial.println();

        NotifyChar->setValue(NotifyVal);
        NotifyChar->notify();

        NotifyVal++;

        Serial.println("Notification sent");
    }
}