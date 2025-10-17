#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <WiFi.h>

#if defined(ESP8266)
    #define RECEIVE_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
    #define RECEIVE_ATTR IRAM_ATTR
#else
    #define RECEIVE_ATTR
#endif

#define samplesize 2000

// Radio config

// ASK/OOK
float frequency = 433.92;
int mod = 2;
float rxbw = 58;
float datarate = 5;
float deviation = 0;

// 2-FSK
/*float frequency = 433.92;
int mod = 0;
float rxbw = 812.50;
float datarate = 5;
float deviation = 47.6;
int powerjammer = -5;
float freqjammer = 433.80;*/

int RXPin = 26;
int RXPin0 = 4;
int TXPin0 = 2;
int TXPin = 25;

//Pushbutton Pins
int push1 = 34;
int push2 = 35;
int pushbutton1 = 0;
int pushbutton2 = 0;

int samplecount;
unsigned long sample[samplesize];
int error_toleranz = 200;
unsigned long samplesmooth[samplesize];
unsigned long secondsamplesmooth[samplesize];
long transmit_push[2000];
static unsigned long lastTime = 0;
const int minsample = 30;
int smoothcount=0;
int secondsmoothcount=0;

bool checkReceived(void){
  delay(100);
  if (samplecount >= minsample && micros()-lastTime >100000){
    detachInterrupt(RXPin);
    //detachInterrupt(RXPin);
    return 1;
  }else{
    return 0;
  }
}

void RECEIVE_ATTR receiver() {
  const long time = micros();
  const unsigned int duration = time - lastTime;

  if (duration > 100000){
    samplecount = 0;
  }

  if (duration >= 100){
    sample[samplecount++] = duration;
  }

  if (samplecount>=samplesize){
    detachInterrupt(RXPin);
    //detachInterrupt(RXPin);
    checkReceived();
  }
  if (samplecount == 1 and digitalRead(RXPin) != HIGH){
  samplecount = 0;
  }
  
  lastTime = time;
}

void signalanalyse(){
  #define signalstorage 10

  int signalanz=0;
  int timingdelay[signalstorage];
  float pulse[signalstorage];
  long signaltimings[signalstorage*2];
  int signaltimingscount[signalstorage];
  long signaltimingssum[signalstorage];
  long signalsum=0;

  for (int i = 0; i<signalstorage; i++){
    signaltimings[i*2] = 100000;
    signaltimings[i*2+1] = 0;
    signaltimingscount[i] = 0;
    signaltimingssum[i] = 0;
  }
  for (int i = 1; i<samplecount; i++){
    signalsum+=sample[i];
  }

  for (int p = 0; p<signalstorage; p++){

  for (int i = 1; i<samplecount; i++){
    if (p==0){
      if (sample[i]<signaltimings[p*2]){
        signaltimings[p*2]=sample[i];
      }
    }else{
      if (sample[i]<signaltimings[p*2] && sample[i]>signaltimings[p*2-1]){
        signaltimings[p*2]=sample[i];
      }
    }
  }

  for (int i = 1; i<samplecount; i++){
    if (sample[i]<signaltimings[p*2]+error_toleranz && sample[i]>signaltimings[p*2+1]){
      signaltimings[p*2+1]=sample[i];
    }
  }

  for (int i = 1; i<samplecount; i++){
    if (sample[i]>=signaltimings[p*2] && sample[i]<=signaltimings[p*2+1]){
      signaltimingscount[p]++;
      signaltimingssum[p]+=sample[i];
    }
  }
  }
  
  int firstsample = signaltimings[0];
  
  signalanz=signalstorage;
  for (int i = 0; i<signalstorage; i++){
    if (signaltimingscount[i] == 0){
      signalanz=i;
      i=signalstorage;
    }
  }

  for (int s=1; s<signalanz; s++){
  for (int i=0; i<signalanz-s; i++){
    if (signaltimingscount[i] < signaltimingscount[i+1]){
      int temp1 = signaltimings[i*2];
      int temp2 = signaltimings[i*2+1];
      int temp3 = signaltimingssum[i];
      int temp4 = signaltimingscount[i];
      signaltimings[i*2] = signaltimings[(i+1)*2];
      signaltimings[i*2+1] = signaltimings[(i+1)*2+1];
      signaltimingssum[i] = signaltimingssum[i+1];
      signaltimingscount[i] = signaltimingscount[i+1];
      signaltimings[(i+1)*2] = temp1;
      signaltimings[(i+1)*2+1] = temp2;
      signaltimingssum[i+1] = temp3;
      signaltimingscount[i+1] = temp4;
    }
  }
  }

  for (int i=0; i<signalanz; i++){
    timingdelay[i] = signaltimingssum[i]/signaltimingscount[i];
  }

  if (firstsample == sample[1] and firstsample < timingdelay[0]){
    sample[1] = timingdelay[0];
  }


  bool lastbin=0;
  for (int i=1; i<samplecount; i++){
    float r = (float)sample[i]/timingdelay[0];
    int calculate = r;
    r = r-calculate;
    r*=10;
    if (r>=5){calculate+=1;}
    if (calculate>0){
      if (lastbin==0){
        lastbin=1;
      }else{
      lastbin=0;
    }
      if (lastbin==0 && calculate>8){
        Serial.print(" [Pause: ");
        Serial.print(sample[i]);
        Serial.println(" samples]");
      }else{
        for (int b=0; b<calculate; b++){
          Serial.print(lastbin);
        }
      }
    }
  }
  Serial.println();
  Serial.print("Samples/Symbol: ");
  Serial.println(timingdelay[0]);
  Serial.println();
 
  smoothcount=0;
  for (int i=1; i<samplecount; i++){
    float r = (float)sample[i]/timingdelay[0];
    int calculate = r;
    r = r-calculate;
    r*=10;
    if (r>=5){calculate+=1;}
      if (calculate>0){
        samplesmooth[smoothcount] = calculate*timingdelay[0];
        smoothcount++;
      }
    }
    Serial.println("Rawdata corrected:");
    Serial.print("Count=");
    Serial.println(smoothcount+1);
  
    for (int i=0; i<smoothcount; i++){
      Serial.print(samplesmooth[i]);
      Serial.print(",");
      transmit_push[i] = samplesmooth[i];
    }
  
  Serial.println();
  Serial.println();
  return;
}

void enableReceive(){
  pinMode(RXPin,INPUT);
  RXPin = digitalPinToInterrupt(RXPin);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(RXPin, receiver, CHANGE);
  samplecount = 0;
}

void printReceived(){
  ELECHOUSE_cc1101.setSidle();
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setSyncMode(0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.setModulation(mod);
  ELECHOUSE_cc1101.setRxBW(rxbw);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(deviation);
  ELECHOUSE_cc1101.setDRate(datarate);

  if(mod == 2) {
    ELECHOUSE_cc1101.setDcFilterOff(0);
  }

  if(mod == 0) {
    ELECHOUSE_cc1101.setDcFilterOff(1);
  }
  
  
  Serial.print("Count=");
  Serial.println(samplecount);
  
  for (int i = 1; i<samplecount; i++){
    Serial.print(sample[i]);
    Serial.print(",");
  }
  Serial.println();
  Serial.println();
}

void setup() {
  //delay(2000);
  // put your setup code here, to run once:
  Serial.begin(38400);
  WiFi.mode(WIFI_STA);
  //delay(1000);
  Serial.print(",");
  ELECHOUSE_cc1101.addSpiPin(14, 12, 13, 27, 1); // Evil Crow RF V2
  //ELECHOUSE_cc1101.addSpiPin(18, 19, 23, 27, 1); // Evil Crow RF V1
  
  pinMode(push1, INPUT);
  pinMode(push2, INPUT);

  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setSyncMode(0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.setModulation(mod);
  ELECHOUSE_cc1101.setRxBW(rxbw);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(deviation);
  ELECHOUSE_cc1101.setDRate(datarate);

  if(mod == 2) {
    ELECHOUSE_cc1101.setDcFilterOff(0);
  }

  if(mod == 0) {
    ELECHOUSE_cc1101.setDcFilterOff(1);
  }
  enableReceive();
}

void replayfirstsignal(){

  ELECHOUSE_cc1101.setSidle();
  
  pinMode(TXPin,OUTPUT);
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(mod);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(deviation);
  ELECHOUSE_cc1101.SetTx();
  delay(100);

  for (int i = 0; i<2000; i+=2){
    digitalWrite(TXPin,HIGH);
    delayMicroseconds(samplesmooth[i]);
    digitalWrite(TXPin,LOW);
    delayMicroseconds(samplesmooth[i+1]);
  }
  delay(1000);
  ELECHOUSE_cc1101.setSidle();
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setSyncMode(0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.setModulation(mod);
  ELECHOUSE_cc1101.setRxBW(rxbw);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(deviation);
  ELECHOUSE_cc1101.setDRate(datarate);

  if(mod == 2) {
    ELECHOUSE_cc1101.setDcFilterOff(0);
  }

  if(mod == 0) {
    ELECHOUSE_cc1101.setDcFilterOff(1);
  }
  enableReceive();
}

void loop() {
  // put your main code here, to run repeatedly:
  pushbutton1 = digitalRead(push1);
  pushbutton2 = digitalRead(push2);
  
  if(checkReceived()){
    printReceived();
    signalanalyse();
    enableReceive();
  }

  if (pushbutton1 == LOW) {
   replayfirstsignal();
  }

  if (pushbutton2 == LOW) {
   replayfirstsignal();
  }
}
