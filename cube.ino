#include <CurieBLE.h>
#include "CurieIMU.h"

BLEPeripheral blePeripheral;
BLEService plotterService("13333333333333333333333333333331");
BLECharacteristic CoordChar("00000000000000000000000072646372", BLERead | BLENotify, 2);
BLEIntCharacteristic BuzzerChar("00000000000000000000000072646322", BLERead | BLEWrite);

int BuzzerPin = 3;
int oldSide = 0;
long previousMillis = 0;

void setup() {
 
  // put your setup code here, to run once:
  Serial.begin(9600);
  CurieIMU.begin();
  CurieIMU.setAccelerometerRange(2);
  pinMode(13, OUTPUT);

  blePeripheral.setLocalName("PlotterCoordSketch");
  blePeripheral.setAdvertisedServiceUuid(plotterService.uuid());
  blePeripheral.addAttribute(plotterService);
  blePeripheral.addAttribute(CoordChar);
  blePeripheral.addAttribute(BuzzerChar);

  //BuzzerChar.setValue({0},1);

  blePeripheral.begin();
  Serial.println("Bluetooth ACTIVATE!");
}

void loop() {
  // put your main code here, to run repeatedly:
  //blePeripheral.poll();
  BLECentral central = blePeripheral.central();


  if(central)
  {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(13,HIGH); 
    while(central.connected())
    {
      long currentMillis = millis();
      if(currentMillis - previousMillis >= 200)
      {
        previousMillis = currentMillis;
        updateCoord();
      }

      
      if(BuzzerChar.written())
      {
        
          Serial.print(BuzzerChar.value());
        if(BuzzerChar.value() == 49) {
          Serial.print("Buzzer on");
            tone(BuzzerPin,43);
        }
        else {
          Serial.print("Buzzer off");
          noTone(BuzzerPin);
        }
      }
    }

    digitalWrite(13,LOW);
    Serial.println("Disconnected from central");
  }
}




void updateCoord() {
  int axRaw, ayRaw, azRaw;         // raw accelerometer values
  float ax, ay, az;
  int side = 0;

  CurieIMU.readAccelerometer(axRaw, ayRaw, azRaw);
  
  ax = convertRawAcceleration(axRaw);
  ay = convertRawAcceleration(ayRaw);
  az = convertRawAcceleration(azRaw);

  if(ax > 0.8)
    side = 1;
  else if(ax < -0.8)
    side = 2;

  if(ay > 0.8)
    side = 3;
  else if(ay < -0.8)
    side = 4;

if(az > 0.8)
    side = 5;
  else if(az < -0.8)
    side = 6;

  const unsigned char coordCharArray[2] = {0, (char)side};
  if(oldSide!= side)
  {
    CoordChar.setValue(coordCharArray, 2);
  }
  oldSide = side;
} 

float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;

  return a;
}