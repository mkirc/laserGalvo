
#include <SPI.h>
#include "MCP48x2.h"

#define MAX_VALUE 4095
#define PIN_INC 10
#define PIN_DEC 11
#define CSPIN 12

byte channelA = CHANNEL_A | GAIN_2 | MODE_ACTIVE;
byte channelB = CHANNEL_B | GAIN_2 | MODE_ACTIVE;

// read step pwm signals from esp32 using external interrrupts
int xStepPin = 2;
int yStepPin = 3;
volatile int xPwmVal = 0;
volatile int yPwmVal = 0;

// reads dir signals from esp32
#define xDirPin A2
#define yDirPin A3
int xDirVal = 0;
int yDirVal = 0;

// holds current x,y
int curX = 0;
int curY = 0;

// checks bounds of current position
int boundsCheck(int current) {
  if (current > 4095) {
    return 1;
  }
  else if (current < 0) {
    return 1;
  }
  else {
    return 0;
  }
}


MCP48x2 dac(MCP4822, CSPIN);

void setup() {
  
  // for testing  
  // Serial.begin(115200);
  // pinMode(LED_BUILTIN, OUTPUT);

  // start spi
  SPI.begin();

  // set chan A,B to 0 V
  dac.send(channelA, 0);
  dac.send(channelB, 0); 
  
  // setup step detection pins
  pinMode(xStepPin, INPUT);
  pinMode(yStepPin, INPUT);

  // setup dir detection pins

  pinMode(xDirPin, INPUT);
  pinMode(yDirPin, INPUT);
}



void loop() {

  xPwmVal = digitalRead(xStepPin);
  yPwmVal = digitalRead(yStepPin);
  xDirVal = digitalRead(xDirPin);
  yDirVal = digitalRead(yDirPin);

  
  if (xPwmVal != 0) {
    String xVal = String(xPwmVal);
    if (xDirVal != 0) {
      if (boundsCheck((curX - 1)) == 0) {
        // Serial.println("x: -" + xVal);
        curX -= 1;
        dac.send(channelA, curX);
      }
      else {
        // Serial.println("xOOB");
      }
    }
    else {
      if (boundsCheck((curX + 1)) == 0) {
        // Serial.println("x: " + xVal);
        curX += 1;
        dac.send(channelA, curX);
      }
      else {
        // Serial.println("xOOB");
      }
    }
  }
  
  if (yPwmVal != 0) {
    String yVal = String(yPwmVal);
    if (yDirVal != 0) {
      if (boundsCheck((curY - 1)) == 0) {
        // Serial.println("y: -" + yVal);
        curY -= 1;
        dac.send(channelB, curY);
      }
      else {
        // Serial.println("yOOB!");
      }
    }
    else {
      if (boundsCheck((curY + 1)) == 0) {
        // Serial.println("y: " + yVal);
        curY += 1;
        dac.send(channelB, curY);
      }
      else {
        // Serial.println("yOOB");
      }
    }
  }

}
