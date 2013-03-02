#include <Wire.h>

#include "clockDisplay.h"
#include "font.h"
#include <inttypes.h>

#define GET_TENTH(x)       ((uint8_t) floor(x/10))
#define GET_ONES(x)        ((uint8_t) (x % 10))

clockDisplay disp = clockDisplay(10,11,13);

int tick_counter = 0;

/**
 * used in main loop: detect a new second...
 */
unsigned char previousSecond;

/**
* All external Pins:
*/
#define MPR_IRQ_PIN     2
#define DCF77PIN        3

/**
 * time vars: the time is stored here!
 */
volatile unsigned char ss;
volatile unsigned char mm;
volatile unsigned char hh;
volatile unsigned char day;
volatile unsigned char mon;
volatile unsigned int year;

/**
 * Clock variables 
 */
volatile unsigned char DCFSignalState = 0;  
unsigned char previousSignalState;
int previousFlankTime;
int saveFlankTime;
int bufferPosition;
unsigned long long dcf_rx_buffer;
bool       doScanSignal = false;

/**
 * Touch state 
 */
boolean touchStates[12]; //to keep track of the previous touch states



void setup()
{
  Serial.begin(115200);
  
  pinMode(MPR_IRQ_PIN, INPUT);
  digitalWrite(MPR_IRQ_PIN, HIGH); //enable pullup resistor
  Wire.begin();
  mpr121_setup();

  
  disp.clear(); // clear display
  disp.setBrightness(0xff);
  int8_t idx,idy=1;
  
  for (idx=11; idx>-19; idx--)
  {
    disp.write(idx, 0, fontLetterc);
    disp.write(idx+6, 0, fontLetterl, 0xffff);    
    disp.write(idx+12, 0, fontLettero, 0xffff);
    disp.write(idx+18, 0, fontLetterC, 0xffff);
    disp.write(idx+24, 0, fontLetterK, 0xffff);
    disp.sync();
    delay(110);
  }  
  delay(1500);
  dispFade(0x0f, 0, 20);
  disp.clear();
  disp.sync();        
  
//  pinMode(DCF77PIN, INPUT);
//  digitalWrite(DCF77PIN, HIGH);
//  int confidence = 0;
//  while (confidence < 10) {
//    DCFSignalState = digitalRead(DCF77PIN);
//    if (DCFSignalState != previousSignalState) {
//      if (DCFSignalState) {
//        confidence = getConfidenceLevel();
//        Serial.println("confidence");
////        digitalWrite(BLINKPIN, HIGH);
//      }
//      else {
////        digitalWrite(BLINKPIN, LOW);
//      }
//      previousSignalState = DCFSignalState;
//    }
//  }
//  previousFlankTime = millis();
  DCF77Init();
  Serial.println("in Sync!");
  
  // set time
  uint8_t  timeSetState = 0;
  uint8_t tmpIdx    =  0;
  uint8_t tmpIdxMax = 23;
  uint8_t  ones     =  0;
  uint8_t  tenth    =  0;
  
  disp.write(0, 0, fontBigNum[0], fontBigNum[0]);
  disp.sync();
  
  while (timeSetState<2)
  {
    readTouchInputs();
    
    if (touchStates[0])
    {
      tmpIdx++;
      if (tmpIdx > tmpIdxMax)
        tmpIdx = 0;

      tenth = (uint8_t) floor(tmpIdx/10);
      ones  = tmpIdx % 10;
      disp.write(0, 0, fontBigNum[tenth], fontBigNum[ones]);
      disp.sync();
      delay(200);
    }
    if (touchStates[1])
    {
      switch (timeSetState)
      {
        case 0:
          hh        = tmpIdx;
          tmpIdxMax = 59;
          for (idx=0; idx>-15; idx--)
          {
            disp.write(idx, 0, fontBigNum[tenth]);
            disp.write(idx+6, 0, fontBigNum[ones], 0xffff);    
            disp.write(idx+12, 0, fontColon, 0xffff);
            disp.write(idx+14, 0, fontBigNum[0], 0xffff);
            disp.write(idx+20, 0, fontBigNum[0], 0xffff);
            disp.sync();
            delay(110);
          }  
          break;
        case 1:
          mm  = tmpIdx;
          for (idx=0; idx>-12; idx--)
          {
            disp.write(idx, 0, fontBigNum[tenth]);
            disp.write(idx+6, 0, fontBigNum[ones], 0xffff);    
            disp.sync();
            delay(110);
            disp.setBrightness(0);
          }          
          break;
        default:
         break;
      }
      tmpIdx = 0;
      ones   = 0;
      tenth  = 0;
      timeSetState++;
      delay(300);
      disp.write(0, 0, fontBigNum[0], fontBigNum[0]);
      disp.sync();
    }
  }
  ss = 0;
  disp.clear();
  disp.sync();
  displaytime(); 
  disp.setBrightness(0x11);
}

void loop()
{
  int sigOk;
  
  uint16_t sensorValue = constrain(analogRead(A1), 700, 1000);
  uint8_t  brightness  = map(sensorValue, 700, 1000, 15, 1);

  disp.setBrightness(0xf0 | brightness);
  if (ss != previousSecond) {
    serialDumpTime();
    Serial.print("Sensor: ");
    Serial.print(analogRead(A1));
    Serial.print(" Brightness: ");
    Serial.println(brightness);
    previousSecond = ss;
    if ((ss == 1)) {
     displaytime(); 
    }
  }
  
  readTouchInputs();
  if (touchStates[0] && touchStates[1])
  {
    dispTime(brightness);
    touchStates[0] = false;
    touchStates[1] = false;
    displaytime();   
    dispFade(0, brightness, 20);
  }
//  if (doScanSignal)
//  {
//    sigOk = scanSignal();
//  }
//  if ((DCFSignalState != previousSignalState)) {
//    previousSignalState = DCFSignalState;
//    sigOk = scanSignal();
////    if (DCFSignalState && doBlink && sigOk) {
////      digitalWrite(BLINKPIN, HIGH);
////    } else {
////      digitalWrite(BLINKPIN, LOW);
////    }
////    if (sigOk > 0) {
////      previousSignalState = DCFSignalState;
////    }
//  }
  
}

void displaytime(void){
  int dispH = hh;
  uint16_t timeBuffer[10];

  // start by clearing the display to a known state
  for (int i=0; i<10; i++) {
    timeBuffer[i] = 0;
  }
  ES_IST;
  
  Serial.print("Es ist ");
  if ((mm>4) && (mm<10)) {
    MFUENF;
    NACH;
    Serial.print("fuenf nach ");
  }
  if ((mm>9) && (mm<15)) {
    MZEHN;
    NACH;
    Serial.print("zehn nach ");
  }
  if ((mm>14) && (mm<20)) {
    VIERTEL;
    Serial.print("viertel ");
  }
  if ((mm>19) && (mm<25)) {
    MZEHN;
    VOR;
    HALB;
    Serial.print("zehn vor halb ");
  }
  if ((mm>24) && (mm<30)) {
    MFUENF;
    VOR;
    HALB;
    Serial.print("fuenf vor halb ");
  }
  if ((mm>29) && (mm<35)) {
    HALB;
    Serial.print("halb ");
  }
  if ((mm>34) && (mm<40)) {
    MFUENF;
    NACH;
    HALB;
    Serial.print("fuenf nach halb ");
  }
  if ((mm>39) && (mm<45)) {
    MZEHN;
    NACH;
    HALB;
    Serial.print("zehn nach halb ");
  }
  if ((mm>44) && (mm<50)) {
    DREIVIERTEL;
    Serial.print("dreiviertel ");
  }
  if ((mm>49) && (mm<55)) {
    MZEHN;
    VOR;
    Serial.print("zehn vor ");
  }
  if (mm>54) {
    MFUENF;
    VOR;
    Serial.print("fuenf vor ");
  }  
  if (mm>14) {
    dispH++;
  }
  dispH = dispH % 12;
  switch (dispH) {
  case 1: 
    EINS;
    Serial.print("Eins ");
    break;
  case 2: 
    ZWEI;
    Serial.print("Zwei ");
    break;
  case 3:
    DREI;
    Serial.print("Drei ");
    break;
  case 4:
    VIER;
    Serial.print("Vier ");
    break;
  case 5:
    FUENF;
    Serial.print("Fuenf ");
    break;
  case 6:
    SECHS;
    Serial.print("Sechs ");
    break;
  case 7:
    SIEBEN;
    Serial.print("Sieben ");
    break;
  case 8:
    ACHT;
    Serial.print("Acht ");
    break;
  case 9:
    NEUN;
    Serial.print("Neun ");
    break;
  case 10:
    ZEHN;
    Serial.print("Zehn ");
    break;
  case 11:
    ELF;
    Serial.print("Elf ");
    break;
  case 0:
    ZWOELF;
    Serial.print("Zwoelf ");
    break;
  }
  if (mm<5) {
    UM;
    Serial.print("Uhr ");
  }  
  Serial.print("\n");
  disp.write(0, 0, timeBuffer);
  disp.write(mm);
  disp.sync();
}

void dumpDebugData(uint16_t rowData) {
  Serial.print("Data: ");
  printBits(rowData);
  Serial.println("");
}

void debugPrint(uint8_t reg, uint8_t val)
{
  Serial.print("Register: ");
  Serial.print(reg);
  Serial.print(" Data: ");
  Serial.println(val);
}

void printBits(uint16_t myWord){
  for(uint16_t mask = 0x8000; mask; mask >>= 1){
    if(mask & myWord)
      Serial.print('1');
    else
      Serial.print('0');
  }
}

