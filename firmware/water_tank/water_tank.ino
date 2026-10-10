// Automatic water tank monitoring and pump control
// Board: Arduino Uno   Sensor: HC-SR04   Display: 16x2 LCD (parallel)

#include <LiquidCrystal.h>

// ---- Pins ----------------------------------------------------
const int TRIG_PIN = 2;          // HC-SR04 trigger
const int ECHO_PIN = 3;          // HC-SR04 echo

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(8, 9, 4, 5, A0, 7);

const int RELAY_PIN = 11;        // drives the 2N2222 that switches the relay
const int GREEN_LED = 12;        // pump running
const int RED_LED   = 13;        // pump off
const int BUZZER    = A1;        // tank full

// ---- Settings ------------------------------------------------
// Distance from the sensor to the tank floor, in cm.
// 30.0 for the real tank (400.0 was used in Proteus to match its sensor model).
const float TANK_HEIGHT = 30.0;
const int LOW_LEVEL  = 30;      // pump ON at or below this level (%)
const int FULL_LEVEL = 90;      // pump OFF at or above this level (%)

// Remembers the pump state between readings (this gives the hysteresis).
bool pumpOn = false;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Safe start-up state: pump off, red LED on, buzzer off.
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(BUZZER, LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Water Level");
  delay(1000);
  lcd.clear();
}

void loop() {
  // 1. Send a 10 microsecond pulse to start a measurement.
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 2. Time the echo (give up after 30 ms).
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // 3. No echo: do not guess. Force the pump off and show an error.
  if (duration == 0) {
    pumpOn = false;
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Sensor error    ");
    lcd.setCursor(0, 1);
    lcd.print("Pump: OFF       ");
    delay(1000);
    return;
  }

  // 4. Echo time to distance in cm (speed of sound 0.0343 cm per microsecond,
  //    divided by 2 because the sound goes out and back).
  float distance = duration * 0.0343 / 2;

  // 5. Distance to level percentage, limited to 0..100.
  float waterLevel = ((TANK_HEIGHT - distance) / TANK_HEIGHT) * 100;
  if (waterLevel < 0)   waterLevel = 0;
  if (waterLevel > 100) waterLevel = 100;

  // 6. Hysteresis: between the two thresholds the pump keeps its last state.
  if (waterLevel <= LOW_LEVEL)  pumpOn = true;
  if (waterLevel >= FULL_LEVEL) pumpOn = false;

  bool tankFull = (waterLevel >= FULL_LEVEL);

  // 7. Set the outputs.
  digitalWrite(RELAY_PIN, pumpOn);   // pump
  digitalWrite(GREEN_LED, pumpOn);   // green = pump running
  digitalWrite(RED_LED, !pumpOn);    // red = pump off
  digitalWrite(BUZZER, tankFull);    // buzzer = tank full

  // 8. Update the LCD. Writing over the old text avoids flicker.
  lcd.setCursor(0, 0);
  lcd.print("Water: ");
  lcd.print((int)waterLevel);
  lcd.print("%   ");

  lcd.setCursor(0, 1);
  if (tankFull)    lcd.print("Tank FULL       ");
  else if (pumpOn) lcd.print("Pump: ON        ");
  else             lcd.print("Pump: OFF       ");

  delay(1000);                       // one reading per second
}
