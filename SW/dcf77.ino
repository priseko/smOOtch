//#define DCF_DEBUG

/**
 * Where is the LED connected?
 */
#define BLINKPIN 13
/**
 * Turn debugging on or off
 */
//#define DCF_DEBUG 1

/**
 * Uncomment if you use an Arduino with ATMEGA168
 */
#define ATMEGA168

/**
 * Number of milliseconds to elapse before we assume a "1",
 * if we receive a falling flank before - its a 0.
 */
#define DCF_split_millis 140
#define DCF_minimum_millis 70
/**
 * There is no signal in second 59 - detect the beginning of 
 * a new minute.
 */
#define DCF_sync_millis    1200
#define DCF_SYNC_PULSES      30
#define DCF_SHORT_PULSES    150
/**
 * Definitions for the timer interrupt 2 handler:
 * The Arduino runs at 16 Mhz, we use a prescaler of 64 -> We need to 
 * initialize the counter with 6. This way, we have 1000 interrupts per second.
 * We use tick_counter to count the interrupts.
 */
#define INIT_TIMER_COUNT 6
#define RESET_TIMER2 TCNT2 = INIT_TIMER_COUNT

#define SAMPLE_COUNT_MAX 250

#define MAX_FLANK_BUFFER_POS 200

long int flankBuffer[MAX_FLANK_BUFFER_POS];
int flankBufferPos = 0;

bool       inDecoding   = false;
uint16_t   sampleCount  = 0;
uint16_t   dcfPulses    = 0;

/**
 * DCF time format struct
 */
struct DCF77Buffer {
  unsigned long long prefix	:21;
  unsigned long long Min	:7;	// minutes
  unsigned long long P1		:1;	// parity minutes
  unsigned long long Hour	:6;	// hours
  unsigned long long P2		:1;	// parity hours
  unsigned long long Day	:6;	// day
  unsigned long long Weekday	:3;	// day of week
  unsigned long long Month	:5;	// month
  unsigned long long Year	:8;	// year (5 -> 2005)
  unsigned long long P3		:1;	// parity
};
struct {
	unsigned char parity_flag	:1;
	unsigned char parity_min		:1;
	unsigned char parity_hour	:1;
	unsigned char parity_date	:1;
} flags;

// hardware constants
int LEDClockPin=6;
int LEDDataPin=7;
int LEDStrobePin=8;
int PWMpin = 9;

/**
 * Initialize the DCF77 routines: initialize the variables,
 * configure the interrupt behaviour.
 */
void DCF77Init() {
  previousSignalState=0;
  previousFlankTime=0;
  bufferPosition=0;
  dcf_rx_buffer=0;
  ss=mm=hh=day=mon=year=0;
#ifdef DCF_DEBUG 
  Serial.println("Initializing DCF77 routines");
  Serial.print("Using DCF77 pin #");
  Serial.println(DCF77PIN);
  pinMode(BLINKPIN, OUTPUT);
#endif
#ifdef DCF_DEBUG
  Serial.println("Initializing timerinterrupt");
#endif
  //Timer2 Settings: Timer Prescaler /64, 
#ifdef ATMEGA168
  TCCR2B |= (1<<CS22);    // turn on CS22 bit
  TCCR2B &= ~((1<<CS21) | (1<<CS20));    // turn off CS21 and CS20 bits   
#else
  TCCR2 |= (1<<CS22);    // turn on CS22 bit
  TCCR2 &= ~((1<<CS21) | (1<<CS20));    // turn off CS21 and CS20 bits   
#endif
  // Use normal mode
#ifdef ATMEGA168
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));   // turn off WGM21 and WGM20 bits 
  TCCR2B &= ~(1<<WGM22);                  // turn off WGM22
#else
  TCCR2 &= ~((1<<WGM21) | (1<<WGM20));   // turn off WGM21 and WGM20 bits 
#endif
  // Use internal clock - external clock not used in Arduino
  ASSR |= (0<<AS2);
#ifdef ATMEGA168
  TIMSK2 |= (1<<TOIE2) | (0<<OCIE2A);        //Timer2 Overflow Interrupt Enable  
#else
  TIMSK |= (1<<TOIE2) | (0<<OCIE2);        //Timer2 Overflow Interrupt Enable  
#endif
  RESET_TIMER2;
#ifdef DCF_DEBUG
  Serial.println("Initializing DCF77 signal listener interrupt");
#endif  
  attachInterrupt(1, dcfSigHandler, RISING);
}


/**
 * Append a signal to the dcf_rx_buffer. Argument can be 1 or 0. An internal
 * counter shifts the writing position within the buffer. If position > 59,
 * a new minute begins -> time to call finalizeBuffer().
 */
void appendSignal(unsigned char signal) {
#if 1
  Serial.print(", appending value ");
  Serial.print(signal, DEC);
  Serial.print(" at position ");
  Serial.println(bufferPosition);
#endif
  dcf_rx_buffer = dcf_rx_buffer | ((unsigned long long) signal << bufferPosition);
  // Update the parity bits. First: Reset when minute, hour or date starts.
  if (bufferPosition ==  21 || bufferPosition ==  29 || bufferPosition ==  36) {
	flags.parity_flag = 0;
  }
  // save the parity when the corresponding segment ends
  if (bufferPosition ==  28) {flags.parity_min = flags.parity_flag;};
  if (bufferPosition ==  35) {flags.parity_hour = flags.parity_flag;};
  if (bufferPosition ==  58) {flags.parity_date = flags.parity_flag;};
  // When we received a 1, toggle the parity flag
  if (signal == 1) {
    flags.parity_flag = flags.parity_flag ^ 1;
  }
  bufferPosition++;
  if (bufferPosition > 59) {
    finalizeBuffer();
  }
}

/**
 * Evaluates the information stored in the buffer. This is where the DCF77
 * signal is decoded and the internal clock is updated.
 */
void finalizeBuffer(void) {
  if (bufferPosition == 59) {
#ifdef DCF_DEBUG
    Serial.println("Finalizing Buffer");
#endif
    struct DCF77Buffer *rx_buffer;
    rx_buffer = (struct DCF77Buffer *)(unsigned long long)&dcf_rx_buffer;
    if (flags.parity_min == rx_buffer->P1  &&
        flags.parity_hour == rx_buffer->P2  &&
        flags.parity_date == rx_buffer->P3) 
    { 
//#ifdef DCF_DEBUG
      Serial.println("Parity check OK - updating time.");
//#endif
      //convert the received bits from BCD
      mm = rx_buffer->Min-((rx_buffer->Min/16)*6);
      hh = rx_buffer->Hour-((rx_buffer->Hour/16)*6);
      day= rx_buffer->Day-((rx_buffer->Day/16)*6); 
      mon= rx_buffer->Month-((rx_buffer->Month/16)*6);
      year= 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6);
    }
#ifdef DCF_DEBUG
      else {
        Serial.println("Parity check NOK - running on internal clock.");
    }
#endif
  } 
  // reset stuff
  ss = 0;
  bufferPosition = 0;
  dcf_rx_buffer=0;
}

/**
 * Dump the time to the serial line.
 */
void serialDumpTime(void){
  Serial.print("    Time: ");
  Serial.print(hh, DEC);
  Serial.print(":");
  Serial.print(mm, DEC);
  Serial.print(":");
  Serial.print(ss, DEC);
  Serial.print(" Date: ");
  Serial.print(day, DEC);
  Serial.print(".");
  Serial.print(mon, DEC);
  Serial.print(".");
  Serial.println(year, DEC);
}

void dcfSigHandler ()
{
  // re-read signal  
  DCFSignalState = digitalRead(DCF77PIN);
  int thisFlankTime=millis();
  
  // check if valid pulse and not in decoding stage
  if ((DCFSignalState) && ((thisFlankTime - previousFlankTime) > 950) && !inDecoding)
  {
    // check if we need to finalize the buffer:
    if (thisFlankTime - previousFlankTime > DCF_sync_millis) {
        Serial.println("####");
        Serial.println("#### Begin of new Minute!!!");
        Serial.println("####");
        
        finalizeBuffer();
    }
    
    // enable decoding stage
    inDecoding = true;
    dcfPulses  = 0;
    
    // save flank time
    previousFlankTime = thisFlankTime;    
  }
}

/**
 * Evaluates the signal as it is received. Decides whether we received
 * a "1" or a "0" based on the 
 */
bool ignoreNextRising = false;

int scanSignal(void){ 
  doScanSignal = false;
  if (dcfPulses <= DCF_SYNC_PULSES)
  {
    // sync gap detected
    Serial.print("dcfPulses: ");
    Serial.print(dcfPulses);
    Serial.println("New Minute!");
    finalizeBuffer();
    
    return 1;
  } else if (dcfPulses <= DCF_SHORT_PULSES)
  {
    appendSignal(0);
    return 1;
  } else 
  {
    appendSignal(1);
    return 1;
  }
#if 0 
    if ((DCFSignalState == 1)  && (!ignoreNextRising)) {
      int thisFlankTime=millis();
      if (thisFlankTime - previousFlankTime > DCF_sync_millis) {
#ifdef DCF_DEBUG
        Serial.println("####");
        Serial.println("#### Begin of new Minute!!!");
        Serial.println("####");
#endif
        finalizeBuffer();
      }
      if ((thisFlankTime - previousFlankTime) < 980) {
        previousSignalState = 0;
        return -1;
      }
//#ifdef DCF_DEBUG
      Serial.print(thisFlankTime - previousFlankTime);
      Serial.print(": DCF77 Signal detected, ");
//#endif
      previousFlankTime=thisFlankTime;
      saveFlankTime = previousFlankTime;
    } 
    else {
      /* or a falling flank */
      int difference=millis() - previousFlankTime;
//#ifdef DCF_DEBUG
      Serial.print("duration: ");
      Serial.print(difference);
//#endif
// eliminate short pulses (noise)
      if (difference > DCF_minimum_millis) { 
        if (difference < DCF_split_millis) {
          appendSignal(0);
        } 
        else {
          appendSignal(1);
        }
        ignoreNextRising = false;
      } else {
        ignoreNextRising = true;
        previousFlankTime = saveFlankTime;
      }
    }
    return 1;
#endif
}

/**
 * The interrupt routine for counting seconds - increment hh:mm:ss.
 */
ISR(TIMER2_OVF_vect) {
  RESET_TIMER2;
  
//  // check if we need to sample some data
//  if (inDecoding)
//  {
//    // read signal
//    DCFSignalState = digitalRead(DCF77PIN);
//    dcfPulses     += DCFSignalState;
//    
//    sampleCount++;
//    
//    if (sampleCount > SAMPLE_COUNT_MAX)
//    {
//      inDecoding   = false;
//      doScanSignal = true;
//      sampleCount  = 0;
//    }
//  }
  
  tick_counter += 1;
  if (tick_counter == 1000) {
    ss++;
    if (ss==60) {
      ss=0;
      mm++;
      Serial.println("MINUTE");
      if (mm==60) {
        mm=0;
        hh++;
        if (hh==24) 
          hh=0;
      }
    }
    tick_counter = 0;
  }
};

/**
 * Interrupthandler for INT0 - called when the signal on Pin 2 changes.
 */
void int0handler() {
  // check the value again - since it takes some time to
  // activate the interrupt routine, we get a clear signal.
  DCFSignalState = digitalRead(DCF77PIN);
}

int getConfidenceLevel(void) {
  int idx, idx2;
  int confidence = 0;
  long int currTime = millis();
  flankBuffer[flankBufferPos] = currTime;
  flankBufferPos++;
  
  if (flankBufferPos>MAX_FLANK_BUFFER_POS) {
    flankBufferPos = 0;
  }
  if (flankBufferPos<30) {
    return -1;
  } 
  
  for (idx=(flankBufferPos-2); idx>0; idx--) {
    for(idx2=1; idx2<30; idx2++) {
      if (((currTime-idx2*1000) > (flankBuffer[idx]-10)) && 
      ((currTime-idx2*1000) < (flankBuffer[idx]+10))) {
        confidence++;
      }
    }
  }

  return confidence;
}




