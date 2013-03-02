/******************************************************************************
 * Includes
 ******************************************************************************/
#include "clockDisplay.h"

#include <inttypes.h>
#include <stdlib.h>
#include "Arduino.h"  

extern void dumpDebugData(uint16_t rowData);
extern void debugPrint(uint8_t reg, uint8_t val);

/******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEBUG_DISP 1

/******************************************************************************
 * Constructors
 ******************************************************************************/

clockDisplay::clockDisplay(uint8_t data, uint8_t clock, uint8_t load)
{
  // record pins for sw spi
  _pinData = data;
  _pinClock = clock;
  _pinLoad = load;

  // set ddr for sw spi pins
  pinMode(_pinClock, OUTPUT);
  pinMode(_pinData, OUTPUT);
  pinMode(_pinLoad, OUTPUT);

  // allocate screenbuffers
  _buffer = (uint16_t*)calloc(10, sizeof(uint16_t));

  // initialize registers
  clear();             // clear display
  setScanLimit(0x07);  // use all rows/digits
  setBrightness(0xff); // maximum brightness 
  setRegister(CONFIG_REG, 0x4d);   // normal operation + individual brightness ctrl
  setRegister(DIGIT_TYPE, 0);
  setRegister(DECODE_MODE, 0);
  setRegister(DISPLAY_TEST, 0x0); // not in test mode
}

/******************************************************************************
 * MAX7219 SPI
 ******************************************************************************/

// sends a single byte by sw spi (no latching)
void clockDisplay::putByte(uint8_t data)
{
  uint8_t i = 8;
  uint8_t mask;
  while(i > 0) {
    mask = 0x01 << (i - 1);         // get bitmask
    digitalWrite(_pinClock, LOW);   // tick
    if (data & mask){               // choose bit
      digitalWrite(_pinData, HIGH); // set 1
    }else{
      digitalWrite(_pinData, LOW);  // set 0
    }
    digitalWrite(_pinClock, HIGH);  // tock
    --i;                            // move to lesser bit
  }
}

// sets register to a byte value for all screens
void clockDisplay::setRegister(uint8_t reg, uint8_t data)
{
  digitalWrite(_pinLoad, LOW); // begin
    putByte(reg);  // specify register
    putByte(data); // send data
  digitalWrite(_pinLoad, HIGH);  // latch in data
  digitalWrite(_pinLoad, LOW); // end
  //debugPrint(reg,data);
}

// syncs row of display with buffer
void clockDisplay::syncRow(uint8_t row)
{
  if ((!_buffer) || (row >= 10)) return;
  
  switch (row) {
    case 0:
      WRITE_D0;
      WRITE_D1;
      break;
    case 1:
      WRITE_D1;
      WRITE_D2;
      break;    
    case 2:
      WRITE_D2;
      WRITE_D3;
      WRITE_D4;
      break;   
    case 3:
      WRITE_D4;
      WRITE_D5;
      break;     
    case 4:
      WRITE_D5;
      WRITE_D6;
      break;     
    case 5:
      WRITE_D6;
      WRITE_D7;
      WRITE_D8;
      break;     
    case 6:
      WRITE_D8;
      WRITE_D9;
      break;   
    case 7:
      WRITE_D9;
      WRITE_D10;
      break;  
    case 8:
      WRITE_D11;
      WRITE_D12;
      break;
    case 9:
      WRITE_D12;
      WRITE_D13;
      break;      
  }
}

/******************************************************************************
 * MAX7219 Configuration
 ******************************************************************************/

// sets how many digits are displayed
void clockDisplay::setScanLimit(uint8_t value)
{
  setRegister(SCAN_LIMIT, value & 0x07);
}

// sets brightness of the display
void clockDisplay::setBrightness(uint8_t value)
{
  uint8_t disp_intensity   = ((value & 0x0E) | ((value & 0x0E) << 4));

  setRegister(INTENS_10, disp_intensity);
  setRegister(INTENS_32, disp_intensity);
  setRegister(INTENS_54, disp_intensity);
  setRegister(INTENS_76, value & 0xEE);
  
  setRegister(INTENS_10A, disp_intensity);
  setRegister(INTENS_32A, disp_intensity);
  setRegister(INTENS_54A, disp_intensity);
  setRegister(INTENS_76A, value & 0xEE);
}

/******************************************************************************
 * Helper Functions
 ******************************************************************************/

void clockDisplay::buffer(uint8_t x, uint8_t y, uint8_t value)
{
  if (!_buffer) return;
  
  // uint8_t's can't be negative, so don't test for negative x and y.
  if (x >= 11 || y >= 10) return;

  // record value in buffer
  if(value){
    _buffer[y] |= 0x8000 >> x;
  }else{
    _buffer[y] &= ~(0x8000 >> x);
  }
}

void clockDisplay::prepBuffer(int8_t x, int8_t y, uint8_t* pBufferStart, uint8_t* pBufferEnd) {
  *pBufferEnd   = 10;
  *pBufferStart = y;
  if (y<0) {
    *pBufferStart = 0;
    *pBufferEnd  += y;
    for (uint8_t i = *pBufferEnd; i < 10; i++) {
      _buffer[i] = 0;
    }
  }
  else {
    for (uint8_t i = 0; i < y; i++) {
      _buffer[i] = 0;
    }
  }
}

uint8_t clockDisplay::shift4to0(uint8_t val)
{
  uint8_t tmpH, tmpL;
  tmpH = (val>>1);
  tmpH = tmpH | ((tmpH & 0x80) | (val & 0x08)<<4);
  
  tmpL = val & 0x07;
  tmpL = tmpL | ((tmpL & 0x08) | (val & 0x10)>>1);

  return ((tmpH & 0xf0) | (tmpL & 0x0f));
}

uint8_t clockDisplay::shift7to0(uint8_t val)
{  
  uint8_t tmp;
  tmp = (val>>1);
  tmp = tmp | ((tmp & 0x80) | (val & 0x01)<<7);

  return tmp;
}

/******************************************************************************
 * User API
 ******************************************************************************/

// buffers and writes to screen
void clockDisplay::write(int8_t minute)
{
  uint8_t val;
  switch (minute % 5)
  {
    case 1:
      val = 0x40;
      break;
    case 2:
      val = 0x60;
      break;
    case 3:
      val = 0x70;
      break;
    case 4:
      val = 0xf0;
      break;
    default: 
      val = 0x0;
      break;
  }
    setRegister(D14, val);
}

void clockDisplay::write(int8_t x, int8_t y, uint8_t value)
{
  buffer(x, y, value);
  
  // update affected row
  syncRow(y);
}

void clockDisplay::write(int8_t x, int8_t y, uint16_t *newBuffer, uint16_t append /*=0*/)
{
  if (y>10) return;
  uint8_t bufferStart, bufferEnd;
  prepBuffer(x, y, &bufferStart, &bufferEnd);
  if (y<0) {
    newBuffer -= y;
  }
  for (uint8_t i = bufferStart; i < bufferEnd; i++){
    if (x<0) {
      _buffer[i] = (append & _buffer[i]) | *newBuffer<<-x;
    }else{
      _buffer[i] = (append & _buffer[i]) | *newBuffer>>x;      
    }
    newBuffer++;
  }
}

void clockDisplay::write(int8_t x, int8_t y, uint16_t *leftDigit, uint16_t *rightDigit, int8_t offset /*=6*/)
{
  if (y>10) return;
  uint8_t bufferStart, bufferEnd;
  prepBuffer(x, y, &bufferStart, &bufferEnd);
  if (y<0) {
    leftDigit  -= y;
    rightDigit -= y;
  }
  for (uint8_t i = bufferStart; i < bufferEnd; i++){
    if (x<0) {
      _buffer[i] = (*leftDigit<<-x) | (*rightDigit<<(-x+offset));
    }else{
      _buffer[i] = (*leftDigit>>x) | (*rightDigit>>(x+offset));   
    }
    leftDigit++;
    rightDigit++;
  }
}

void clockDisplay::write(int8_t x, int8_t y, uint16_t *leftDigit, uint16_t *middleDigit, uint16_t *rightDigit, int8_t offset  /*=6*/)
{
  if (y>10) return;
  uint8_t bufferStart, bufferEnd;
  prepBuffer(x, y, &bufferStart, &bufferEnd);
  if (y<0) {
    leftDigit   -= y;
    middleDigit -= y;
    rightDigit  -= y;
  }
  for (uint8_t i = bufferStart; i < bufferEnd; i++){
    if (x<0) {
      _buffer[i] =  (*leftDigit<<-x) | (*middleDigit<<(-x+offset));// | (*rightDigit<<(-x+(2*offset)));
    }else{
      _buffer[i] =  (*leftDigit>>x)  | (*middleDigit>>(x+offset));// | (*rightDigit>>(x+(2*offset)));   
    }
    leftDigit++;
    middleDigit++;
    rightDigit++;
  }
}

void clockDisplay::getBuffer() {
  for (uint8_t i=0; i<10; i++) {
    dumpDebugData(_buffer[i]);
  }
}

void clockDisplay::getBuffer(uint8_t reg) {
    dumpDebugData(_buffer[reg]);
}

//syncs complete display
void clockDisplay::sync()
{
  WRITE_D0;
  WRITE_D1;
  WRITE_D2;
  WRITE_D3;
  WRITE_D4;
  WRITE_D5;
  WRITE_D6;
  WRITE_D7;
  WRITE_D8;
  WRITE_D9;
  WRITE_D10;
  WRITE_D11;
  WRITE_D12;
  WRITE_D13;
}

// clears screens and buffers
void clockDisplay::clear(void)
{
  if (!_buffer) return;

  // clear buffer
  for(uint8_t i = 0; i < 10; ++i){
      _buffer[i] = 0x0000;
  }

  write(0);
  // clear registers
  for(uint8_t i = 0; i < 10; ++i){
    syncRow(i);
  }
}

