/* MeterOS v1.0 by delphijustin
 * https://delphijustin.biz
 * For temperature calibration see the thermistors directory of this respository
 * 
*/

#include "meterAPI.h"
#include "dongwa-oled.h" //dongwa logo
void serialEvent() {
  String S = Serial.readString();
  if (S.indexOf((char)0x08) > -1) {
    SerialCommand = "";
    return;
  }
  S.toLowerCase();
  SerialCommand += S;
  if (SerialCommand.indexOf((char)0x0a) > -1 || SerialCommand.indexOf((char)0x0d) > -1) {
    if (SerialCommand.indexOf("vdc") > -1) {
      mode = 0;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("adc") > -1) {
      mode = 1;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("rul") > -1) {
      mode = 2;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("rl") > -1) {
      mode = 3;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("temp") > -1) {
      mode = 4;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("stat") > -1) {
      mode = 5;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("pnjunction") > -1) {
      mode = 6;
      level = 0;
      t = 0;
    }
    if (SerialCommand.indexOf("resetlevel") > -1)level = 0;
    if (SerialCommand.indexOf("resetspeed") > -1)t = 0;
    if (SerialCommand.indexOf("level+") > -1)level++;
    if (SerialCommand.indexOf("level-") > -1)level--;
    if (SerialCommand.indexOf("speed+") > -1)t--;
    if (SerialCommand.indexOf("speed-") > -1)t++;
    if (SerialCommand.indexOf("level=") > -1 && SerialCommand.indexOf(";") > -1) {
      SerialCommand.replace("level=", ""); SerialCommand.replace(";", ""); level = SerialCommand.toInt();
    }
    if (SerialCommand.indexOf("speed=") > -1 && SerialCommand.indexOf(";") > -1) {
      SerialCommand.replace("speed=", ""); SerialCommand.replace(";", ""); t = SerialCommand.toInt() % 512;
    }

  }
  t = abs(t);
}
void setup() {
  //CALIBRATED_THERMISTOR_HERE
  pinMode(NEGITIVEV_PIN, INPUT);
  pinMode(BUTTON_SHIFT, INPUT_PULLUP);
  pinMode(BUTTON_MODE_UP, INPUT_PULLUP);
  pinMode(BUTTON_MODE_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEVEL_UP, INPUT_PULLUP);
  pinMode(BUTTON_LEVEL_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_HOLD, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(FIVEFIVEFIVE_TRIGGER, OUTPUT);
  pinMode(FIVEFIVEFIVE_RESET, OUTPUT);
  pinMode(FIVEFIVEFIVE_INPUT, INPUT);

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  // Clear the buffer
  display.clearDisplay();  // Clear the display buffer
  display.drawBitmap(0, 0, dongwaoled, 128, 32, WHITE);
  display.display();  // Push the buffer to the display
  delay(2048);
  display.clearDisplay();

  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner
  display.print("delphijustin MeterOS");
  display.setCursor(0, SYMBOL_SIZE);  // Start at top-left corner
  display.print("Version 1.0");
  display.setCursor(0, SYMBOL_SIZE * 2);  // Start at top-left corner
  display.print("www.delphijustin.biz");

  display.display();
  delay(2048);
  Serial.println("METER_READY");
  display.clearDisplay();
  lastTicks = millis();
  attachInterrupt(digitalPinToInterrupt(NEGITIVEV_PIN), incCycles, FALLING);
}
void loop() {
  unsigned long currentTicks = millis();
  if (currentTicks - lastTicks > 999) {
    hz = cycles;
    cycles = 0;
    lastTicks = currentTicks;
  }
  switch (getButton()) {
    case BUTTON_SPEED_UP:
      t--;
      t = abs(t);
      delay(1024);
      break;
    case BUTTON_SPEED_DOWN:
      t++;
      delay(1024);
      break;
    case BUTTON_MODE_UP:
      mode++;
      noTone(buzzerPin);
      display.clearDisplay();
      level = 0; t = 0;
      delay(1024);
      break;
    case BUTTON_MODE_DOWN:
      mode--; t = 0;
      mode = abs(mode);
      noTone(buzzerPin);
      display.clearDisplay();
      level = 0;
      delay(1024);
      break;
    case BUTTON_HOLD:
      onHold = !onHold;
      delay(1024);
      break;
    case BUTTON_LEVEL_UP:
      level++;
      delay(1024);
      break;
    case BUTTON_LEVEL_DOWN:
      level--;
      delay(1024);
      break;
  }
  if (onHold) return;
  if (t % 512 > 0)delay(t);
  String line1 = "";
  String line2 = "";
  String line3 = "";
  String line4 = "";
  LoopCount++;
  char StrBuffer[32];
  float R1;
  unsigned long capStart;
  switch (mode % Mode_Count) {
    case 0:
      if (FEATURES & FEATURE_VOLTMETER < 1) {
        mode++; break;
      }
      line1 = String(voltMeterRead(AtoV(analogRead(POSITIVEV_PIN))) + BridgeDrop, 4) + "Vdc";
      line2 = "Min=" + String(Vmin, 4) + "Vdc";
      line3 = "Max=" + String(Vmax, 4) + "Vdc";
      line4 = String(hz) + "Hz " + String(1.0 / hz, 6) + "s";
      break;
    case 1:
      if (FEATURES & FEATURE_AMMETER < 1) {
        mode++; break;
      }
      line1 = String(AtoV(analogRead(AMPMeter)) / AMPShunt, 6) + "Adc";
      break;
    case 2:
    case 3:
      if (FEATURES & FEATURE_OHMMETER < 1) {
        mode++; break;
      }
      R1 = ((RMETER_SHUNT * 5.0) / AtoV(analogRead(RESISTOR_PIN))) - OHMS_OFFSET;
      line1 = String(R1, 4) + "Ohms";
      line2 = String(AtoV(analogRead(RESISTOR_PIN)), 4) + "V";
      if (abs((level + 1) * 10) % 100 < analogRead(RESISTOR_PIN)) {
        tone(buzzerPin, map(analogRead(RESISTOR_PIN), 0, 1023, 31, 4096));
        line4 = "BUZZ!";
      } else {
        if (mode % Mode_Count == 2) {
          noTone(buzzerPin);
        }
        line4 = "";
      }
      line4 += "(" + String(abs(level * 10) % 100) + "," + String(analogRead(RESISTOR_PIN)) + ")" + OhmModes[(mode % Mode_Count) - 2];
      break;
    case 4:
      if (FEATURES & FEATURE_TEMP < 1) {
        mode++; break;
      }
      line1="Thermistor not calibrated";
      line2="Run thermistor tool";
      if(!thermistor.calibrated)break;
      R1 = ((RMETER_SHUNT * 5.0) / AtoV(analogRead(RESISTOR_PIN))) - OHMS_OFFSET;
      line1 = String(KtoF(1.0/(thermistor.m * log(R1) + thermistor.b)),2) + "deg. F";
      line2 = String(KtoC(1.0/(thermistor.m * log(R1) + thermistor.b)), 2) + "deg. C";
      line3 = String(1.0/(thermistor.m * log(R1) + thermistor.b), 2) + "deg. K";
      line4 = String(R1)+"Ohms";
      break;
      if (FEATURES & FEATURE_STAT < 1) {
        mode++; break;
      }
      line1 = String(AtoV(analogRead(POSITIVEV_PIN)) + BridgeDrop, 6) + "Vdc";
      line2 = String(AtoV(analogRead(AMPMeter)) / AMPShunt, 6) + "Adc";
      line3 = String((AtoV(analogRead(POSITIVEV_PIN)) + BridgeDrop) / (AtoV(analogRead(AMPMeter)) / AMPShunt), 6) + "Ohms";
      line4 = String((AtoV(analogRead(POSITIVEV_PIN)) + BridgeDrop) * (AtoV(analogRead(AMPMeter)) / AMPShunt), 6) + "Wdc";
      break;
    case 5:
      if (FEATURES & (FEATURE_CAPACITANCE_555) < 1) {
        mode++; break;
      }
      display.clearDisplay();
      display.setTextSize(1);               // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);  // Draw white text
      display.setCursor(0, 0);
      display.print(CHARGING_CAP);
      display.display();
      Serial.println(CHARGING_CAP);
      for (byte I = 0; I < 3; I++)Serial.println();
      digitalWrite(FIVEFIVEFIVE_RESET, LOW); delay(15);
      digitalWrite(FIVEFIVEFIVE_RESET, HIGH); delay(15);
      capStart = micros();
      digitalWrite(FIVEFIVEFIVE_TRIGGER, LOW); delay(15);
      digitalWrite(FIVEFIVEFIVE_TRIGGER, HIGH); delay(15);
      while (digitalRead(FIVEFIVEFIVE_INPUT) == HIGH) {
        display.setTextSize(1);               // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE);  // Draw white text
        display.setCursor(0, SYMBOL_SIZE);
        Serial.println(CHARGING_CAP);
        line2 = String(fivefivefive(micros() - capStart) * 1e6, 6) + "uF";
        Serial.println(line2);
        line2.toCharArray(StrBuffer, 32);
        for (byte I = 0; I < 2; I++)Serial.println();
        display.print(StrBuffer);
        display.display();
      }
      line2 = String(fivefivefive(micros() - capStart) * 1e6, 6) + "uF";

      break;
  }
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  for (byte ypx = 0; ypx < 4; ypx++) {
    display.setCursor(0, ypx * SYMBOL_SIZE);  // Start at top-left corner
    switch (ypx) {
      case 0:
        line1.toCharArray(StrBuffer, 32);
        break;
      case 1: line2.toCharArray(StrBuffer, 32); break;
      case 2:
        line3.toCharArray(StrBuffer, 32);
        break;
      case 3: line4.toCharArray(StrBuffer, 32); break;
    }
    display.print(StrBuffer);
  }
  display.display();
  Serial.println(line1);
  Serial.println(line2);
  Serial.println(line3);
  Serial.println(line4);
}
