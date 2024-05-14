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

#define PWM_LEVEL_STARTING 250
#define PWM_LEVEL_DEFAULT 100

#define TARGET_ROTATION_FREQ_33 34
#define TARGET_ROTATION_FREQ_45 108

#define DATA_ARRAY_SIZE 0x20
#define SAMPLING_FREQ 1000
#define STARTING_TIME 2
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
unsigned int zeroIndex = 0;
unsigned int duration = 0;
int secCounter = 0;
int zeroCounter = 0;
int prevData;
int rotationFreq;
int potentiometer;
int targetRotationFreq = TARGET_ROTATION_FREQ_33;
int zero_level;
byte pwmLevel = 150;

void setup() {
  pinMode(AUTO_STOP_PIN, INPUT);
  pinMode(3, OUTPUT);
  Serial.begin(115200);
  Timer1.initialize(timerConst); 
  Timer1.attachInterrupt(timerInterrupt);
  mode = READY;
}

void loop() {
  switch (mode) {
    
    case READY:
        if (potentiometer > 10) mode = STARTING;
    break;
    case STARTING:
        analogWrite(MOTOR_PIN, PWM_LEVEL_STARTING);
//        targetRotationFreq = digitalRead(RPM_PIN) == LOW ? TARGET_ROTATION_FREQ_33 : TARGET_ROTATION_FREQ_45;
    break;
    case PLAYING:
/*        byte minv = 148;
        byte maxv = 156;
        if (rotationFreq > 37 ) pwmLevel--;
        if (pwmLevel < minv) pwmLevel = minv;
        if (rotationFreq < 37) pwmLevel++;
        if (pwmLevel > maxv) pwmLevel = maxv;
        analogWrite(MOTOR_PIN, pwmLevel);*/
        if (potentiometer < 10) {
          mode = READY;
          pwmLevel = 0;
          analogWrite(MOTOR_PIN, pwmLevel);    
        }
    break;
  }
  
  if (dataready)
  {
    zero_level = GetZeroLevel();

    dataAver = GetAver() - zero_level;
    if (dataAver * prevData < 0 || dataAver == 0) 
    {
      duration = (mainIndex - zeroIndex) & DATA_ARRAY_SIZE - 1;
      zeroIndex = mainIndex;
      zeroCounter++;
    }
    prevData = dataAver;

    low = dataAver & 0xFF;
    high  = dataAver >> 8;
    Serial.write(25);
    Serial.write(low);
    Serial.write(high);
    low = rotationFreq & 0xFF;
    high  = rotationFreq >> 8;
    low = duration & 0xFF;
    high  = duration >> 8;
    Serial.write(low);
    Serial.write(high);
    dataready = false;
  }
}

void timerInterrupt(){
     potentiometer = analogRead(5);
  
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
          if (startCounter == 2)
          {
            startCounter = 0;
            pwmLevel = 122;
            mode = PLAYING;
          }
        }
        rotationFreq = zeroCounter;

        if (mode == PLAYING)
        {
          byte minv = 148;
          byte maxv = 156;
          if (rotationFreq > 30 ) pwmLevel--;
          if (pwmLevel < minv) pwmLevel = minv;
          if (rotationFreq < 30) pwmLevel++;
          if (pwmLevel > maxv) pwmLevel = maxv;
          analogWrite(MOTOR_PIN, pwmLevel);
        }
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
//   return (int)sum / AVER_SIZE;
  return dataArray[mainIndex];
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
