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

/**********************************************************
 * Function: receiveEvent
 *********************************************************/
void receiveEvent(int num)
{
   static int count = BUF_SIZE/2;
   for (int j=0; j<num; j++) {
      buffer[count] = Wire.read();
      count = (count + 1) % BUF_SIZE;
   }
}

// --------------------------------------
// Handler function: requestEvent
// --------------------------------------
void requestEvent()
{
}

/**********************************************************
 * Function: play_bit
 *********************************************************/
void play_bit() 
{
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
}

/**********************************************************
 * Function: check if we have to mute 
 *********************************************************/
int isMute(){
  int value = digitalRead(7);
  if(value == 0){
    playback = 0;
  } else if(value == 1){
    playback = 1;
  }
  return 1;
}

/**********************************************************
 * Function: check if we have to turn on the led 
 *********************************************************/
int turnOnLed(){
  digitalWrite(13, (1-playback));
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
}

/**********************************************************
 * Function: loop
 *********************************************************/
void loop ()
{
    unsigned long timeDiff;

    isMute();
    turnOnLed();
    play_bit();

    timeDiff = SAMPLE_TIME - (micros() - timeOrig);
    timeOrig = timeOrig + SAMPLE_TIME;
    delayMicroseconds(timeDiff);
}
