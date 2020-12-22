/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <Wire.h>
#include "let_it_be_1bit.h"

/**********************************************************
 *  CONSTANTS
 *********************************************************/

// UNCOMMENT THIS LINE TO EXECUTE WITH RASPBERRY PI 
//#define RASPBERRYPI 1

#define SLAVE_ADDR 0x8

#define SAMPLE_TIME 250 
#define SOUND_PIN  11
#define BUF_SIZE 256

/**********************************************************
 *  GLOBALS
 *********************************************************/

unsigned char buffer[BUF_SIZE];
unsigned long timeOrig;
int playback = 1;
int pushed = 0;



/**********************************************************
 * Function: receiveEvent
 *********************************************************/
void receiveEvent(int num)
{
    double start = micros();
   static int count = BUF_SIZE/2;
   for (int j=0; j<num; j++) {
      buffer[count] = Wire.read();
      count = (count + 1) % BUF_SIZE;
   }
  double endLoop = micros();
}

// --------------------------------------
// Handler function: requestEvent
// --------------------------------------
void requestEvent()
{
}



/**********************************************************
 * Function: check if we have to mute 
 *********************************************************/
int isMute(){
  int value = 0;
  value = digitalRead(7); 
  if(value == 1 && pushed == 0) {
    pushed  = 1;
  }
  else if(pushed == 1 && value == 0){
     pushed = 0;
     if(playback == 0){
       playback = 1;
     } else if (playback == 1){
       playback = 0;
     }
  }
  return 0;
}

/**********************************************************
 * Function: check if we have to turn on the led 
 *********************************************************/
int turnOnLed(){
  digitalWrite(13, (1-playback));
  return 0;
}

/**********************************************************
 * Function: setup
 *********************************************************/
void setup ()
{
    // Initialize I2C communications as Slave
    Wire.begin(SLAVE_ADDR); 
    // Function to run when data requested from master
    Wire.onRequest(requestEvent);
    // Function to run when data received from master
    Wire.onReceive(receiveEvent);
    // set I2C speed to 1 MHz
    Wire.setClock(1000000L);

    pinMode(SOUND_PIN, OUTPUT);
    memset (buffer, 0, BUF_SIZE);
    timeOrig = micros();
    
    TCCR1A = 0;// set entire TCCR1A register to 0 
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0;
    // set timer count for khz increments
    OCR1A = 499;// = (16*10^6) / (4000*8) - 1
    // turn on CTC mode
    // Set CS11 bit for 8 prescaler (there is a table) 
    TCCR1B = _BV(WGM12) | _BV(CS11);
    // enable timer compare interrupt
    TIMSK1 = _BV(OCIE1A);

}

/**********************************************************
 * Function: play_bit
 *********************************************************/

ISR (TIMER1_COMPA_vect)
{  
  double start = micros();
  static int bitwise = 1;
  static unsigned char data = 0;
  static int music_count = 0;
  bitwise = (bitwise * 2);
  if (bitwise > 128) {
      bitwise = 1;
      #ifdef RASPBERRYPI 
        data = buffer[music_count];
        music_count = (music_count + 1) % BUF_SIZE;
      #else 
        data = pgm_read_byte_near(music + music_count);
        music_count = (music_count + 1) % MUSIC_LEN;
      #endif
  }
  if(playback == 1){
    digitalWrite(SOUND_PIN, (data & bitwise) );
  }
  double endLoop = micros();
}

/**********************************************************
 * Function: loop
 *********************************************************/
void loop ()
{
    unsigned long timeDiff;

    isMute();
    turnOnLed();

    timeDiff = SAMPLE_TIME - (micros() - timeOrig);
    timeOrig = timeOrig + SAMPLE_TIME;
    delayMicroseconds(timeDiff);
}
