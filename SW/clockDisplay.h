/*
  Matrix.h - Max7219 LED Matrix library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef clockDisplay_h
#define clockDisplay_h

#include <inttypes.h>

// Matrix registers
#define NO_OP         0x00
#define DECODE_MODE   0x01
#define GLO_INTENSITY 0x02
#define SCAN_LIMIT    0x03
#define CONFIG_REG    0x04
#define GPIO_DATA     0x05
#define PORT_CONFIG   0x06
#define DISPLAY_TEST  0x07
#define DIGIT_TYPE    0x0C
#define INTENS_10     0x10
#define INTENS_32     0x11
#define INTENS_54     0x12
#define INTENS_76     0x13
#define INTENS_10A    0x14
#define INTENS_32A    0x15
#define INTENS_54A    0x16
#define INTENS_76A    0x17

#define D0P0  0x20
#define D0aP0 0x28
#define D1P0  0x21
#define D1aP0 0x29
#define D2P0  0x22
#define D3P0  0x23
#define D4P0  0x24
#define D5P0  0x25
#define D6P0  0x26
#define D7P0  0x27

#define D0P1  0x40
#define D0aP1 0x48
#define D1P1  0x41
#define D1aP1 0x49
#define D2P1  0x42
#define D3P1  0x43
#define D4P1  0x44
#define D5P1  0x45
#define D6P1  0x46
#define D7P1  0x47

#define D0  0x60
#define D1  0x68
#define D2  0x61
#define D3  0x69
#define D4  0x62
#define D5  0x6A
#define D6  0x63
#define D7  0x6B
#define D8  0x64
#define D9  0x6C
#define D10 0x65
#define D11 0x6D
#define D12 0x66
#define D13 0x6E
#define D14 0x67

#define D0_DEF  (_buffer[0]>>8)
#define D1_DEF  (_buffer[0] & 0x00e0)    | (_buffer[1]>>11)
#define D2_DEF  (_buffer[1] & 0x07e0)>>3 | (_buffer[2]>>14)
#define D3_DEF  (_buffer[2] & 0x3fc0)>>6
#define D4_DEF  (_buffer[2] & 0x0020)<<2 | (_buffer[3]>>9)
#define D5_DEF  (_buffer[3] & 0x01e0)>>1 | (_buffer[4]>>12)
#define D6_DEF  (_buffer[4] & 0x0fe0)>>4 | (_buffer[5]>>15)
#define D7_DEF  (_buffer[5] & 0x7f80)>>7
#define D8_DEF  (_buffer[5] & 0x0060)<<1 | (_buffer[6]>>10)
#define D9_DEF  (_buffer[6] & 0x03e0)>>2 | (_buffer[7]>>13)
#define D10_DEF (_buffer[7]>>5)
#define D11_DEF (_buffer[8]>>8)
#define D12_DEF (_buffer[8] & 0x00e0) | (_buffer[9]>>11)
#define D13_DEF (_buffer[9] & 0x07e0)>>3

#define WRITE_D0  setRegister(D0,  shift4to0(D0_DEF))
#define WRITE_D1  setRegister(D1,  shift7to0(D1_DEF))
#define WRITE_D2  setRegister(D2,  shift4to0(D2_DEF))
#define WRITE_D3  setRegister(D3,  shift7to0(D3_DEF))
#define WRITE_D4  setRegister(D4,  shift4to0(D4_DEF))
#define WRITE_D5  setRegister(D5,  shift7to0(D5_DEF))
#define WRITE_D6  setRegister(D6,  shift4to0(D6_DEF))
#define WRITE_D7  setRegister(D7,  shift7to0(D7_DEF))
#define WRITE_D8  setRegister(D8,  shift4to0(D8_DEF))
#define WRITE_D9  setRegister(D9,  shift7to0(D9_DEF))
#define WRITE_D10 setRegister(D10, shift4to0(D10_DEF))
#define WRITE_D11 setRegister(D11, shift7to0(D11_DEF))
#define WRITE_D12 setRegister(D12, shift4to0(D12_DEF))
#define WRITE_D13 setRegister(D13, shift7to0(D13_DEF))

// Clock words
#define ES_IST      timeBuffer[0] |= 0b1101110000000000  
#define MFUENF      timeBuffer[0] |= 0b0000000111100000

#define MZEHN	    timeBuffer[1] |= 0b1111000000000000
#define ZWANZIG     timeBuffer[1] |= 0b0000111111100000

#define DREIVIERTEL timeBuffer[2] |= 0b1111111111100000
#define VIERTEL	    timeBuffer[2] |= 0b0000111111100000

#define VOR	    timeBuffer[3] |= 0b1110000000000000
#define UM	    timeBuffer[3] |= 0b0000110000000000
#define NACH	    timeBuffer[3] |= 0b0000000111100000

#define HALB	    timeBuffer[4] |= 0b1111000000000000
#define ELF	    timeBuffer[4] |= 0b0000011100000000
#define FUENF	    timeBuffer[4] |= 0b0000000111100000

#define EINS	    timeBuffer[5] |= 0b1111000000000000
#define ZWEI	    timeBuffer[5] |= 0b0000000111100000

#define DREI	    timeBuffer[6] |= 0b1111000000000000
#define VIER	    timeBuffer[6] |= 0b0000000111100000

#define SECHS  	    timeBuffer[7] |= 0b1111100000000000
#define ACHT	    timeBuffer[7] |= 0b0000000111100000

#define SIEBEN	    timeBuffer[8] |= 0b1111110000000000
#define ZWOELF	    timeBuffer[8] |= 0b0000001111100000

#define ZEHN	    timeBuffer[9] |= 0b1111000000000000
#define NEUN	    timeBuffer[9] |= 0b0001111000000000
#define UHR         timeBuffer[9] |= 0b0000000011100000

class clockDisplay
{
  private:
    uint8_t _pinData;
    uint8_t _pinClock;
    uint8_t _pinLoad;

    uint16_t* _buffer;

    void putByte(uint8_t);
    void setRegister(uint8_t, uint8_t);
    void syncRow(uint8_t);

    void setScanLimit(uint8_t);

    void buffer(uint8_t, uint8_t, uint8_t);
    void prepBuffer(int8_t, int8_t, uint8_t*, uint8_t*);
    
    uint8_t shift4to0(uint8_t);
    uint8_t shift7to0(uint8_t);
  public:
    clockDisplay(uint8_t, uint8_t, uint8_t);
    void setBrightness(uint8_t);
    void write(int8_t);
    void write(int8_t, int8_t, uint8_t);
    void write(int8_t, int8_t, uint16_t *, uint16_t = 0x0);
    void write(int8_t, int8_t, uint16_t *, uint16_t *, int8_t = 6);
    void write(int8_t, int8_t, uint16_t *, uint16_t *, uint16_t *, int8_t = 6);
    void sync();
    void getBuffer();
    void getBuffer(uint8_t);
    void clear(void);
};

#endif

