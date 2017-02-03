#include <CurieBLE.h>
#include "CurieIMU.h"
#include <CurieTime.h>
#include <EEPROM.h>


#define EPS 0.25
#define MEAN_EXTREME_VAL 1.7
#define ACC_RANGE 4
#define GYRO_RANGE 125
#define TIME_STEP_MILLIS 100
#define ALPHA 0.1
#define BRIGHTNESS 255
#define SWITCH_OFF_SIDE 11

/*
BLEService plotterService("13333333333333333333333333333331");
BLECharacteristic CoordChar("00000000000000000000000072646372", BLERead | BLENotify, 2);
BLEIntCharacteristic BuzzerChar("00000000000000000000000072646322", BLERead | BLEWrite);
*/
BLEPeripheral blePeripheral;

//current side and isActive
BLEService StateService("0F00");
BLEUnsignedIntCharacteristic CurrSideChar("0F01", BLERead | BLENotify);
BLEUnsignedCharCharacteristic IsActiveChar("0F02", BLERead | BLENotify);

//TODO: implement writing IsActiveChar

//Sides configuration
BLEService SideConfService("0F10");
BLECharacteristic SideConfChar("0F11", BLERead | BLEWrite | BLENotify, 12);

//Signals
BLEService SignalService("0F20");
BLEUnsignedCharCharacteristic BuzzerChar("0F21", BLERead | BLEWrite | BLENotify);
BLEDescriptor BuzzerDesc("0E00", "Buzzer");
BLECharacteristic DiodeModeChar("0F22", BLERead | BLEWrite | BLENotify, 12);

void setupPins();
void sideConfWritten();
void buzzerWritten();
void diodeModeWritten();
void sideConfWritten(BLECentral& central, BLECharacteristic& characteristic);
void buzzerWritten(BLECentral& central, BLECharacteristic& characteristic);
void diodeModeWritten(BLECentral& central, BLECharacteristic& characteristic);
int isStaticPosition(int mode, const float d);
int isShake();
void switchOnNotification();
void switchOffNotification();

const byte diod_default_state[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int diodePin[] = {A0, A1, A2, A3, A4, A5, 2, 3, 4, 5, 6, 7};

struct s_time {
  time_t day = 0;
  time_t hour = 0;
  time_t minute = 0;
  time_t second = 0;
} currentTime;

bool diodeOn[12];
int oldSide = 12;
long previousMillis = 0;
float mean = 0;
bool buzzerOn = false;
bool synTime = true;

float normz[] = {
0, 0,
0.27639320225002095, -0.27639320225002095, 
-0.7236067977499792, 0.7236067977499792, 
-0.7236067977499789, 0.7236067977499789, 
0.2763932022500214, -0.2763932022500214, 
0.8944271909999161, -0.8944271909999161
};

float normy[] = {
0, 0,
-0.85065080835204, 0.85065080835204, 
-0.5257311121191335, 0.5257311121191335, 
0.5257311121191339, -0.5257311121191339, 
0.85065080835204, -0.85065080835204, 
0, 0
};

float normx[] = {
1, -1,
0.4472135954999579, -0.4472135954999579, 
0.4472135954999579, -0.4472135954999579, 
0.4472135954999579, -0.4472135954999579, 
0.4472135954999579, -0.4472135954999579, 
0.4472135954999579, -0.4472135954999579
};

void setup() {
  Serial.begin(9600);

  CurieIMU.begin();
  CurieIMU.setAccelerometerRange(ACC_RANGE);
  CurieIMU.setGyroRange(GYRO_RANGE);
  Serial.println("IMO activated.");

  //TODO: ask about descriptors

  //BLE initialization
  blePeripheral.setLocalName("Polytope");
  blePeripheral.setAdvertisedServiceUuid("0F00"); //TODO: verify this is legit

  //StateService
  blePeripheral.addAttribute(StateService);
  blePeripheral.addAttribute(CurrSideChar);
  blePeripheral.addAttribute(IsActiveChar);

  //SideConfService
  blePeripheral.addAttribute(SideConfService);
  blePeripheral.addAttribute(SideConfChar);

  //Signal
  blePeripheral.addAttribute(SignalService);
  blePeripheral.addAttribute(BuzzerChar);
  blePeripheral.addAttribute(DiodeModeChar);

  //TODO: setup initial values for characteristics


  //event handlers
  SideConfChar.setEventHandler(BLEWritten, sideConfWritten);
  BuzzerChar.setEventHandler(BLEWritten, buzzerWritten);
  DiodeModeChar.setEventHandler(BLEWritten, diodeModeWritten);

  setupPins();
  blePeripheral.begin();
  Serial.println("Bluetooth activated.");
  /*Serial.begin(9600);
  CurieIMU.begin();
  CurieIMU.setAccelerometerRange(1);
  CurieIMU.setGyroRange(10);
  //blePeripheral.setLocalName("PlotterCoordSketch");
  blePeripheral.setAdvertisedServiceUuid(plotterService.uuid());
  blePeripheral.addAttribute(plotterService);
  blePeripheral.addAttribute(CoordChar);
  blePeripheral.addAttribute(BuzzerChar);
  blePeripheral.begin();
  Serial.println("Bluetooth ACTIVATE!");
  */
  setupPins();
}

void loop() {
  BLECentral central = blePeripheral.central();
  synTime = false;// true;
  Serial.println("Loop iteration.");
  if(central) {
    if(synTime) {
      // Запрашиваем время.
      // ...
      // Получили время. Устанавливаем с помощью функции setTime(...).
      setTime(12, 0, 0, 1, 2, 2017); // hour, minute, second, day, month, year
      delay(2000);
      currentTime = getSTime();
      synTime = false;
    }

    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(13,HIGH); 
    
    switchOnNotification();
    delay(1000);
    switchOffNotification();

    while(central.connected())
    {
      blePeripheral.poll();
      long currentMillis = millis();

      if(currentMillis - previousMillis > 800) {
        //switchOnNotification();
        if(isStaticPosition(1, 3));
            updateSide();
        previousMillis = currentMillis;
        //delay(1000);
        //switchOffNotification();
      }

      if(isShake()){
        Serial.println("IT SHAKES!");
        BuzzerChar.setValue(false);
        
        byte* default_state = new byte[12];
        for (int i=0; i < 12; i++) default_state[i] = 0;
        const byte * const_default_state = default_state;
//        for (int i=0; i < 12; i++) DiodeModeChar.setValue(&(diod_default_state[0]), 12);
        DiodeModeChar.setValue(&(diod_default_state[0]), 12);
        buzzerWritten();
        diodeModeWritten();
      }
    }
    
    digitalWrite(13,LOW);
    Serial.println("Disconnected from central");
    mean = 0;
}
}

void updateSide() {
  float ax, ay, az;
  int side = 12;

  CurieIMU.readAccelerometerScaled(ax, ay, az);
  //ax /= 1.09;
  //ay /= 1.09;
  //az /= 1.09;
  
  float c_dist = 10;
  for(int s = 0; s < 12; s++) {
    float n_dist = sqrt(pow((normx[s]-ax),2)+pow((normy[s]-ay),2)+pow((normz[s]-az),2));    
    if(n_dist < c_dist) {
      c_dist = n_dist;
      side = s;
    }
  }

  if(c_dist > EPS)
      side = 12;
  
  if(side != oldSide && oldSide != 12) {
    EEPROM_updateSideTime(oldSide);
    
    //const unsigned char coordCharArray[2/**/] = {0, (char)side/*, время*/};
    //CoordChar.setValue(coordCharArray, 2/**/);
    CurrSideChar.setValue((char)side);
    // Отправляем время
    // CurrSideTime.setValue(...);

    currentTime = getSTime();

    s_time p0 = EEPROM_getSideTime(side);
    s_time p1 = EEPROM_getSideTime(oldSide);
    Serial.print("Side: "); Serial.print(side); Serial.print(" | ");
    Serial.print("Time: "); Serial.print(p0.minute); Serial.print(":"); Serial.println(p0.second);
    Serial.print("OldSide: "); Serial.print(oldSide); Serial.print(" | ");
    Serial.print("Time: "); Serial.print(p1.minute); Serial.print(":"); Serial.println(p1.second);
    Serial.println("");
  }

  else if(side != oldSide && oldSide == 12) {
    //const unsigned char coordCharArray[2/**/] = {0, (char)side/*, время*/};
    //CoordChar.setValue(coordCharArray, 2/**/);
    
    CurrSideChar.setValue((char)side);
    // Отправляем время
    // CurrSideTime.setValue(...);

    currentTime = getSTime();

    s_time p0 = EEPROM_getSideTime(side);
    s_time p1 = EEPROM_getSideTime(oldSide);
    Serial.print("~Side: "); Serial.print(side); Serial.print(" | ");
    Serial.print("~Time: "); Serial.print(p0.minute); Serial.print(":"); Serial.println(p0.second);
    Serial.print("~OldSide: "); Serial.print(oldSide); Serial.print(" | ");
    Serial.print("~Time: "); Serial.print(p1.minute); Serial.print(":"); Serial.println(p1.second);
    Serial.println("");
  }

  oldSide = side;
} 

int isShake() {
      float fAccelX, fAccelY, fAccelZ, fAccelAbSq;
      float fOmegaX, fOmegaY, fOmegaZ;
  
      CurieIMU.readAccelerometerScaled(fAccelX, fAccelY, fAccelZ);
      CurieIMU.readGyroScaled(fOmegaX, fOmegaY, fOmegaZ);
      mean *= (1-ALPHA);
      mean += ALPHA * (fAccelX * fAccelX + fAccelY * fAccelY + fAccelZ * fAccelZ);

      if (mean > MEAN_EXTREME_VAL)
          return 1;
      else return 0;
}

int isStaticPosition(int mode, const float d){
  float wx, wy, wz;
  
  CurieIMU.readGyroScaled(wx, wy, wz);

  if(mode == 1)
    if(wx > d || wy > d || wz > d)
      return 0;
  
  if(mode == 2)
    if(wx > d && wy > d && wz > d)
      return 0;

  return 1;
}

int EEPROM_resetSideTime(int i) {
  s_time buf;
  EEPROM.put(i*sizeof(s_time), buf);
  return 1;
}

int EEPROM_updateSideTime(int i) {
  s_time sideTime = EEPROM_getSideTime(i);

  sideTime.second += second() - currentTime.second;
  sideTime.minute += minute() - currentTime.minute;
  sideTime.hour += hour() - currentTime.hour;
  sideTime.day += day() - currentTime.day;
  
  if(sideTime.second < 0){
    sideTime.second += 60;
    sideTime.minute -= 1;
  }
  else if(sideTime.second > 59){
    sideTime.second -= 60;
    sideTime.minute += 1;
  }
  
  if(sideTime.minute < 0){
    sideTime.minute += 60;
    sideTime.hour -= 1;
  } 
  else if(sideTime.minute > 59){
    sideTime.minute -= 60;
    sideTime.hour += 1;
  }

  if(sideTime.hour < 0){
    sideTime.hour += 24;
    sideTime.day -= 1;
  }
  else if(sideTime.hour > 24){
    sideTime.hour -= 24;
    sideTime.day += 1;
  }

  if(sideTime.day < 0 || sideTime.day > 31)
    sideTime.day = 0;
    
  EEPROM.put(i*sizeof(s_time), sideTime);
  
  return 1;
}

s_time EEPROM_getSideTime(int i) {
  s_time val;
  EEPROM.get(i*sizeof(s_time), val);
  return val;
}

int EEPROM_clear() {
  for(int i = 0; i < 512; i++)
    EEPROM.write(i,0);
  return 1;
}

s_time getSTime(){
  s_time t;
  
  t.day = day();
  t.hour = hour();
  t.minute = minute();
  t.second = second();

  return t;
}

void setupPins() {
  for (int i=0; i < 12; i++) 
    pinMode(diodePin[i], OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(13, OUTPUT);
}

void sideConfWritten(BLECentral& central, BLECharacteristic& characteristic) {
  //TODO: implement
 }

void buzzerWritten() {
  Serial.print(F("Characteristic event, written: "));
  bool state = (bool)BuzzerChar.value();

  if (state != buzzerOn) {
    if (state) {
      Serial.println("Buzzer is on");
      digitalWrite(8, HIGH);
      digitalWrite(9, HIGH);
     } else {
      Serial.println("Buzzer is off");
      digitalWrite(8, LOW);
      digitalWrite(9, LOW);
    }
  }
  buzzerOn = state;
}

void buzzerWritten(BLECentral& central, BLECharacteristic& characteristic) {
  buzzerWritten();
}

void diodeModeWritten() {
  Serial.print(F("Characteristic event, written: "));
  const byte* state = DiodeModeChar.value();

  for (int i=0; i < 12; i++) {
    if (state[i] != diodeOn[i]) {
      if (state[i]) {
        Serial.print(F("Diode #"));
        Serial.print(i);
        Serial.println(" is on");
        if (i < 6) {
          analogWrite(diodePin[i], BRIGHTNESS);
        } else {
          digitalWrite(diodePin[i], HIGH);
        }
       } else {
        Serial.print(F("Diode #"));
        Serial.print(i);
        Serial.println(" is off");
        if (i < 6) {
          analogWrite(diodePin[i], 0);
        } else {
          digitalWrite(diodePin[i], LOW);
        }
      }
      diodeOn[i] = state[i];
    }
  }
}

void diodeModeWritten(BLECentral& central, BLECharacteristic& characteristic) {
  diodeModeWritten();
}

void switchOnNotification() {
  for (int i = 0; i < 12; i++) {
    state[i] = true;
  }
  BuzzerChar.setValue(1);
  diodeModeWritten();
  buzzerWritten();
}

void switchOffNotification() {
  byte* default_state = new byte[12];
  for (int i=0; i < 12; i++) default_state[i] = 0;
  const byte * const_default_state = default_state;
//        for (int i=0; i < 12; i++) DiodeModeChar.setValue(&(diod_default_state[0]), 12);
    DiodeModeChar.setValue(&(diod_default_state[0]), 12);
  BuzzerChar.setValue(0);
  diodeModeWritten();
  buzzerWritten();
}
