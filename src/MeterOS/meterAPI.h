/*
    This file contains API and settings for MeterOS
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const byte FEATURE_VOLTMETER = 1;
const byte FEATURE_AMMETER = 2;
const byte FEATURE_OHMMETER = 4;
const byte FEATURE_TEMP = 8;
const byte FEATURE_STAT = 16;
const byte FEATURE_CAPACITANCE_555 = 32;

byte FEATURES = FEATURE_VOLTMETER | FEATURE_AMMETER | FEATURE_OHMMETER | FEATURE_TEMP | FEATURE_STAT | FEATURE_CAPACITANCE_555;
/*
   Features to enable OR (|) them together
   The value 0 stops MeterOS from measuring anything
*/
float RESISTOR_FIVEFIVEFIVE = 1e6; // can be less than 1Meg but shouldn't go over.
byte FIVEFIVEFIVE_TRIGGER = 11; //555 Trigger
byte FIVEFIVEFIVE_RESET = A6; //555 reset pin
byte FIVEFIVEFIVE_INPUT = 12; //connect this pin to 555's output pin
byte RESISTOR_PIN = A1;//ohmmeter analog pin
byte NEGITIVEV_PIN = 2;      //negative voltage detector and frequency counter pin Must be interrupt
byte POSITIVEV_PIN = A0;     //positive voltmeter pin
float RMETER_SHUNT = 150.0;  // R2 of ohm meter voltage divider
const byte BUTTON_MODE_UP = 5;     //Mode up button
const byte BUTTON_MODE_DOWN = 6;   //mode down button
const byte BUTTON_HOLD = 7;        //hold button
const byte BUTTON_LEVEL_UP = 8;    //level up button
const byte BUTTON_LEVEL_DOWN = 9;  //level down button
const byte BUTTON_SHIFT = 10; //shift button
//The button constants below are combinations and not pin numbers
const byte BUTTON_SHIFT_FLAG = 0x80; //Not a button
const byte BUTTON_ZERO = BUTTON_SHIFT_FLAG | BUTTON_MODE_DOWN; //set resistance to 0 value
const byte BUTTON_SPEED_UP = BUTTON_SHIFT_FLAG | BUTTON_LEVEL_UP; //Speed Up
const byte BUTTON_SPEED_DOWN = BUTTON_SHIFT_FLAG | BUTTON_LEVEL_DOWN; //slow Down
const byte BUTTON_RESET = BUTTON_SHIFT_FLAG | BUTTON_HOLD; //reset speed and level settins
float BridgeDrop = 0; //Voltage drop from bridge rectifier
float OHMS_OFFSET = RMETER_SHUNT; //This will be subtracted by the resistance measurement and temperature measurement
byte buzzerPin = 13;         //Ohmmeter speaker out
byte AMPMeter = A2;          //AMMeter pin
float AMPShunt = 1.0;        //AMMeter Shunt in ohms
long level = 0;           //level/range variable
const float MAXfloat = 1e+38;      //max value for floats
const float MINfloat = 1e-38;      //min value for floats
float Vmin = MAXfloat;             //Minimum voltage read
float Vmax = MINfloat;             //maximum voltage read
bool onHold = false;               //Hold state
long t = 0; //delay time between each reading
unsigned long hz = 0;              //frequency
unsigned long cycles = 0;          //cycles before each second
long mode = 0;//Current mode
unsigned long lastTicks;           //milliseconds before last frequency counter update
String SerialCommand = "";         //Serial Terminal command buffer
byte Mode_Count = 6;         //Number of modes
const byte SYMBOL_SIZE = 8;  //Symbol height in pixcels
const String OhmModes[2] = { "UL", "L"};
struct THERMISTOR_DATA {
  bool calibrated;// if true calibfateThermistor does nothing
  float m;//slope
  float b;//y-intercept
};
THERMISTOR_DATA thermistor;
const char CHARGING_CAP[16] = "Charging Cap...";
unsigned long LoopCount = 0;
void calibrateThermistor(float m,float b){
  if(thermistor.calibrated)return;
  thermistor.calibrated=true;
  thermistor.m=m;
  thermistor.b=b;
}
bool inRange(float x, float xmin, float xmax) {
  return (x >= xmin) && (x <= xmax);
}
float AtoV(int a) {  //convert analog value to voltage
  return a * (5 / 1023.0);
}
word VtoA(float v) {
  return trunc(abs(v / (5 / 1023.0)));
}
void incCycles() {  //Increase cycles
  cycles++;
}
int getSign() {
  if (digitalRead(NEGITIVEV_PIN) == LOW)return -1;
  return 1;
}
float voltMeterRead(float x) {
  float y = abs(x);
  Vmin = min(Vmin, y);
  Vmax = max(Vmax, y);
  return y * abs(1 + (level % 6)) * getSign();
}
float KtoC(float K) {
  return K - 273.15;
}
float KtoF(float K) {
  return KtoC(K) * (9.0 / 5) + 32;
}
float fivefivefive(unsigned long t) {
 return (909.091e-3 * (t / 1e+6))/RESISTOR_FIVEFIVEFIVE;
}
byte getButton() {
  if (digitalRead(BUTTON_SHIFT) == LOW)return BUTTON_SHIFT_FLAG | (BUTTON_MODE_UP * (HIGH ^ digitalRead(BUTTON_MODE_UP))) | (BUTTON_MODE_DOWN * (HIGH ^ digitalRead(BUTTON_MODE_DOWN))) | (BUTTON_LEVEL_UP * (HIGH ^ digitalRead(BUTTON_LEVEL_UP))) | (BUTTON_LEVEL_DOWN * (HIGH ^ digitalRead(BUTTON_LEVEL_DOWN))) | (BUTTON_HOLD * (HIGH ^ digitalRead(BUTTON_HOLD)));
  if (digitalRead(BUTTON_MODE_UP) == LOW) return BUTTON_MODE_UP;
  if (digitalRead(BUTTON_MODE_DOWN) == LOW) return BUTTON_MODE_DOWN;
  if (digitalRead(BUTTON_HOLD) == LOW) return BUTTON_HOLD;
  if (digitalRead(BUTTON_LEVEL_UP) == LOW) return BUTTON_LEVEL_UP;
  if (digitalRead(BUTTON_LEVEL_DOWN) == LOW) return BUTTON_LEVEL_DOWN;
  return 0;
}
