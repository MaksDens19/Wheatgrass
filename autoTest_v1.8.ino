// Підключення бібліотек ----------------------------------------------
#define CAYENNE_PRINT Serial
#include <CayenneMQTTEthernet.h>
#include <Servo.h>

// Підключення до облікового запису Cayenne ---------------------------
char username[] = "72132430-e533-11e8-a08c-c5a286f8c00d";
char password[] = "024a8baabe52acc8df85d92a991d4aa37820518c";
char clientID[] = "b8f19710-e538-11e8-a254-e163efaadfe8";

// Надання портам Ардуіно та віртуальним портам Cayenne своєї програмної назви ----------------------------
#define VIRTUAL_CHANNEL_LED 7
#define ACTUATOR_PIN_LED 7

#define VIRTUAL_CHANNEL_DOOR 4
#define ACTUATOR_PIN_DOOR 5

#define VIRTUAL_CHANNEL_SPRAY 8
#define ACTUATOR_PIN_SPRAY 8

#define VIRTUAL_CHANNEL_FAN 9
#define ACTUATOR_PIN_FAN 9

#define SENSOR_PIN_HUMIDITY A0
#define VIRTUAL_CHANNEL_HUMIDITY 0

#define VIRTUAL_CHANNEL_RUN 10
#define VIRTUAL_CHANNEL_PHASE_0 11
#define VIRTUAL_CHANNEL_PHASE_1 12
#define VIRTUAL_CHANNEL_PHASE_2 13
#define ACTUATOR_PIN 13

Servo servo_door; // Підключення серво мотора

// Фоторезистор
const int pinPhoto = A1;
int light_level = 0;

// Current Phase Initiation ---------
// 0 - Пауза перед замочуванням
// 1 - Фаза замочування
// 2 - Пауза перед пророщуванням
// 3 - Фаза пророщування
// 4 - Фаза вегетації
// 5 - Цикл завершено
int current_phase = 0; // Не визначено

int light = 0; // Світло виключене

unsigned long delayMillis; // Затримка перед фазами

unsigned long sprayerFixedTime; // Змінна, яка фіксує час для поливання у фазах

unsigned long fanFixedTime; // Змінна, яка фіксує час для обдуву

unsigned long ledFixedTime; // Змінна, яка фіксує час для включення або виключення світла

// Тривалість кожної з фаз (PRODUCTION)
//const int PHASE_0_DURATION = long(2) * long(60) * long(60) * long(1000);      // 2 години
//const int PHASE_1_DURATION = 3 * 24 * 60 * 60 * 1000; // 3 дня
//const int PHASE_2_DURATION = 5 * 24 * 60 * 60 * 1000; // 5 днів

// Тривалість кожної з фаз (TEST)
const unsigned long PHASE_0_DURATION = long(30) * long(1000); // 30 секунд
const unsigned long PHASE_1_DURATION = long(45) * long(1000); // 45 секунд
const unsigned long PHASE_2_DURATION = long(60) * long(1000); // 60 секунд

// Тривалість перерви між поливанням та обдувом
const unsigned long PHASE_1_2_SPRAYER_PAUSE_DURATION = long(10) * long(60) * long(1000); // 10 хв
const unsigned long PHASE_2_FAN_PAUSE_DURATION = long(13) * long(60) * long(1000); // 13 хв

// LED on and off
//const unsigned long DAY_LIGHT_DURATION = long(18) * long(60)* long(60) * long(1000); // 2 хв
//const unsigned long NIGHT_LIGHT_DURATION = long(6) * long(60)* long(60) * long(1000); // 1 хв

// LED on and off (TEST)
const unsigned long DAY_LIGHT_DURATION = long(2) * long(60) * long(1000); // 2 хв
const unsigned long NIGHT_LIGHT_DURATION = long(1) * long(60) * long(1000); // 1 хв
unsigned long SLP; // Start Light Phase

// Що відбувається після включення Ардуіно:
void setup() {
  Serial.begin(9600);
  Cayenne.begin(username, password, clientID);

  // LED КОНФІГУРАЦІЯ ---------------------------------------
  pinMode(ACTUATOR_PIN_LED, OUTPUT);
  digitalWrite(ACTUATOR_PIN_LED, HIGH);

  // DOOR КОНФІГУРАЦІЯ --------------------------------------------
  servo_door.attach(ACTUATOR_PIN_DOOR);

  // FAN КОНФІГУРАЦІЯ -------------------------------------------
  pinMode(ACTUATOR_PIN_FAN, OUTPUT);
  digitalWrite(ACTUATOR_PIN_FAN, HIGH);

  // WATER SPRAY КОНФІГУРАЦІЯ --------------------------------------
  pinMode(ACTUATOR_PIN_SPRAY, OUTPUT);

  // HUMIDITY SENSOR --------------------------------------
  // Конфігурації не потрібно.

  // LIGHT SENSOR КОНФІГУРАЦІЯ -----------------------------
  pinMode(pinPhoto, INPUT);

  // Cayenne LED button off
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_LED, LOW);

  // Cayenne FAN button off
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_FAN, LOW);

  // Cayenne SPRAY button off
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_SPRAY, LOW);


  // Активація кнопок у Cayenne
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_RUN, HIGH);
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_0, LOW);
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_1, LOW);
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_2, LOW);

}

//  MAIN APPLICATION LOOP ==================================================================================================================
void loop() {
  Cayenne.loop();

  if ((current_phase == 1) && (millis() >= PHASE_0_DURATION + delayMillis)) { // Якщо зараз 1 фаза
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_0, LOW); // та час для закінчення даної фази ще не прийшов, то
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_1, LOW); //Виключити усі кнопки,
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_2, LOW); //що стосуються фаз,
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_RUN, HIGH); // крім кнопки "Run";
    current_phase = 2; // Змінити значення цієї змінної на 2,
    DoorOpen(); // та закрити двері.
  }

  if (current_phase == 3) {
    if (millis() < PHASE_1_DURATION + delayMillis) {
      while ((analogRead(SENSOR_PIN_HUMIDITY) > 610) && (millis() - PHASE_1_2_SPRAYER_PAUSE_DURATION >= sprayerFixedTime)) {
        DoorOpen();
        digitalWrite(ACTUATOR_PIN_SPRAY, LOW); // Включити спрей
        delay(20000);
        digitalWrite(ACTUATOR_PIN_SPRAY, HIGH); // Виключити спрей
        DoorClose();
        sprayerFixedTime = millis();
      }
    }
    else {
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_0, LOW);
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_1, LOW);
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_2, HIGH);
      current_phase = 4;
      DoorOpen();
      delayMillis = millis();
      SLP = millis();
    }
  }

  if (current_phase == 4) {
    if (millis() < PHASE_2_DURATION + delayMillis) {
      while ((analogRead(SENSOR_PIN_HUMIDITY) > 610) && (millis() - PHASE_1_2_SPRAYER_PAUSE_DURATION >= sprayerFixedTime)) {
        DoorOpen();
        digitalWrite(ACTUATOR_PIN_SPRAY, LOW);
        delay(20000);
        digitalWrite(ACTUATOR_PIN_SPRAY, HIGH);
        DoorClose();
        fanFixedTime = millis();
      }
      while (millis() - PHASE_2_FAN_PAUSE_DURATION >= fanFixedTime) {
        digitalWrite(ACTUATOR_PIN_FAN, LOW);
        delay(30000);
        digitalWrite(ACTUATOR_PIN_FAN, HIGH);
        fanFixedTime = millis();
      }
      if (millis() - SLP < DAY_LIGHT_DURATION + NIGHT_LIGHT_DURATION) {
        LedCheck(SLP);
      }
      else {
        SLP = millis();
        LedCheck(SLP);
      }
    }
    else {
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_RUN, HIGH);
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_0, HIGH);
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_1, HIGH);
      Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_2, HIGH);
      current_phase = 5;
    }

  }




}

// DOOR CONTROL FUNCTIONS -------------------------------------------------------
void DoorOpen() { // Відкриває двері
  for (int i = 0; i <= 180; i += 1) {
    servo_door.write(i);
    delay(10);
  }
}

void DoorClose() { // Закриває двері
  for (int i = 180; i >= 0; i -= 1) {
    servo_door.write(i);
    delay(10);
  }
}

// LED check function ===========================================================
void LedCheck(unsigned long SLP) {
  if (millis() - SLP < DAY_LIGHT_DURATION) {
    if (light == 0) {
      digitalWrite(ACTUATOR_PIN_LED, LOW);
      light = 1;
    }
  }
  else {
    if (light == 1) {
      digitalWrite(ACTUATOR_PIN_LED, HIGH);
      light = 0;
    }
  }
}

//---------------------------------------------------------------------- CAYENNE FUNCTIONS

CAYENNE_IN(VIRTUAL_CHANNEL_RUN)
{
  if (current_phase == 0) { // Якщо значення змінної дорівнює 0, то
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_0, HIGH); // Включити на Cayenne-і кнопку Phase 0
    delayMillis = millis(); // Запам'ятати час до цього моменту
    current_phase = 1; // Змінити значення змінної на 1
    DoorClose(); // Та закрити двері (це окрема функція).
  }

  if (current_phase == 2) {
    Cayenne.virtualWrite(VIRTUAL_CHANNEL_PHASE_1, HIGH);
    delayMillis = millis();
    current_phase = 3;
    DoorClose();
  }

}

CAYENNE_IN(VIRTUAL_CHANNEL_DOOR)
{

  // Determine angle to set the servo.
  int position = getValue.asDouble() * 180;
  // Move the servo to the specified position.
  servo_door.write(position);
}

CAYENNE_IN(VIRTUAL_CHANNEL_LED)
{
  if (getValue.asInt() == 0) {
    digitalWrite(ACTUATOR_PIN_LED, HIGH);
  }
  else {
    digitalWrite(ACTUATOR_PIN_LED, LOW);
  }
}

CAYENNE_IN(VIRTUAL_CHANNEL_FAN)
{
  // Write the value received to the digital pin.
  if (getValue.asInt() == 0) {
    digitalWrite(ACTUATOR_PIN_FAN, HIGH);
  }
  else {
    digitalWrite(ACTUATOR_PIN_FAN, LOW);
  }
  //digitalWrite(ACTUATOR_PIN_FAN, value);
}

CAYENNE_IN(VIRTUAL_CHANNEL_SPRAY)
{
  // Write value to turn the relay switch on or off. This code assumes you wire your relay as normally open.
  if (getValue.asInt() == 0) {
    digitalWrite(ACTUATOR_PIN_SPRAY, LOW);
  }
  else {
    digitalWrite(ACTUATOR_PIN_SPRAY, HIGH);
  }
}

CAYENNE_OUT(VIRTUAL_CHANNEL_HUMIDITY)
{
  Cayenne.virtualWrite(VIRTUAL_CHANNEL_HUMIDITY, analogRead(SENSOR_PIN_HUMIDITY));
}
