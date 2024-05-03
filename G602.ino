#include "TimerOne.h"

#define READY 0
#define STARTING 1
#define PLAYING 2

#define MOTOR_PIN 11
#define PHOTORES_PIN A1
#define AUTO_STOP_PIN A3
#define START_PIN 4
#define STOP_PIN 5
#define RPM_PIN 6

#define STOP_LEVEL 20

#define PWM_LEVEL_STARTING 255
#define PWM_LEVEL_DEFAULT 180

#define TARGET_ROTATION_FREQ_33 80
#define TARGET_ROTATION_FREQ_45 108

#define DATA_ARRAY_SIZE 0x20
#define SAMPLING_FREQ 1000
#define STARTING_TIME 3
#define AVER_SIZE 10

#define HYSTERESIS 1

byte mode = READY;
long timerConst = 1000000 / SAMPLING_FREQ;

byte low;
byte high;

byte startCounter = 0;

int data;
int dataAver;

int dataArray[DATA_ARRAY_SIZE];
bool dataready = false;
unsigned int mainIndex = 0;
int secCounter = 0;
int zeroCounter = 0;
int prevData;
int rotationFreq;
int targetRotationFreq;
int zero_level;
byte pwmLevel;

void setup() {
  pinMode(AUTO_STOP_PIN, INPUT);
  Serial.begin(115200);
  Timer1.initialize(timerConst); 
  Timer1.attachInterrupt(timerInterrupt);
}

void loop() {
  switch (mode) {
    case READY:
      if (digitalRead(START_PIN) == HIGH) {
        analogWrite(MOTOR_PIN, PWM_LEVEL_STARTING);
        mode = STARTING;    
      }
    break;
    case STARTING:
        targetRotationFreq = digitalRead(RPM_PIN) == LOW ? TARGET_ROTATION_FREQ_33 : TARGET_ROTATION_FREQ_45;
    break;
    case PLAYING:
      if (rotationFreq > targetRotationFreq + HYSTERESIS) pwmLevel--;
      if (rotationFreq < targetRotationFreq + HYSTERESIS) pwmLevel++;
      if (analogRead(AUTO_STOP_PIN) < STOP_LEVEL || digitalRead(STOP_PIN) == HIGH) {
        analogWrite(MOTOR_PIN, 0);
        mode = READY;
      }
    break;
  }
  if (dataready)
  {
    dataAver = GetAver();
    if (dataAver * prevData < 0) zeroCounter++;
    prevData = dataAver;

    low = (dataAver - zero_level) & 0xFF;
    high  = (dataAver - zero_level) >> 8;
    Serial.write(25);
    Serial.write(low);
    Serial.write(high);
    low = rotationFreq & 0xFF;
    high  = rotationFreq >> 8;
    Serial.write(low);
    Serial.write(high);
    dataready = false;
  }
}

void timerInterrupt(){
     data = analogRead(PHOTORES_PIN);
     dataArray[mainIndex] = data;
     mainIndex++;
     mainIndex = mainIndex & DATA_ARRAY_SIZE - 1;
     dataready = true;

     secCounter++;
     if (secCounter > SAMPLING_FREQ)
     {
        if (mode ==  STARTING)
        {
          startCounter++;
          if (startCounter > STARTING_TIME) 
          {
            mode = PLAYING;
            analogWrite(MOTOR_PIN, PWM_LEVEL_DEFAULT);
            targetRotationFreq = TARGET_ROTATION_FREQ_33;
          }
        }
        zero_level = GetZeroLevel();
        
        rotationFreq = zeroCounter / 2;
        secCounter = 0;
        zeroCounter = 0;
     }
}

int GetAver()
{
   double sum = 0;
   for (int i = 0; i < AVER_SIZE; i++) 
   { 
       sum += dataArray[(mainIndex - i) & DATA_ARRAY_SIZE - 1];
   }
   return (int)sum / AVER_SIZE;
}

int GetZeroLevel()
{
   double sum = 0;
   for (int i = 0; i < DATA_ARRAY_SIZE; i++) 
   { 
       sum += dataArray[i];
   }
   return (int)sum / DATA_ARRAY_SIZE;
}
