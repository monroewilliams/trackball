#include <SPI.h>
#include <avr/pgmspace.h>

#include "adns.h"

/*
  Pin assignments on Sparkfun Pro Micro:
  
  3 - sensor 1 motion (MOT)
  4 - left button
  5 - middle button
  6 - right button
  9 - piezo speaker +
  
  10 - sensor 1 chip select (SS)
  16 - SPI MOSI (MO)
  14 - SPI MISO (MI)
  15 - SPI SCLK (SC)
  
  VCC - sensor VI
  GND - sensor DG & AG
  
*/

byte initComplete = 0;
byte serial_debug = 0;

adns xy_sensor;

// Button state polling/tracking
const int buttonCount = 3;
const char buttonNames[3] = {MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
const int buttonPins[3] = {4, 5, 6};

const int piezo_pin = 9;

const int scroll_tick = 127;
int scroll_accum = 0;

void setup() 
{
  Mouse.begin();
  
  if (serial_debug)
  {
    Serial.begin(9600);
    // Add a short delay so I can get the console open before things start happening.
    delay(2000);
  }
    
  // set up interrupt 0 (pin 3) to deal with the "motion" pin of the x/y sensor
  attachInterrupt(0, UpdatePointer, FALLING);

  SPI.begin();
  
  xy_sensor.init(10);

  for(int i=0; i<buttonCount; i++)
  {
      pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Piezo output
  pinMode(piezo_pin, OUTPUT);
  
  initComplete = 1;
}

void click()
{
  digitalWrite(piezo_pin, HIGH);
  digitalWrite(piezo_pin, LOW);
}

void UpdatePointer(void)
{
    if (initComplete)
    {
      signed char xh, yh;
      unsigned char xl, yl;
      
      xl = xy_sensor.read_reg(REG_Delta_X_L);
      xh = xy_sensor.read_reg(REG_Delta_X_H);
      yl = xy_sensor.read_reg(REG_Delta_Y_L);
      yh = xy_sensor.read_reg(REG_Delta_Y_H);
      
      int x, y;
      x = (((int)xh) << 8) | (unsigned int)xl;
      y = (((int)yh) << 8) | (unsigned int)yl;
      
      if (serial_debug)
      {
          Serial.print("X: ");
          Serial.print(xh,HEX);
          Serial.print(xl,HEX);
          Serial.print(", Y: ");
          Serial.print(yh,HEX);
          Serial.print(yl,HEX);
          Serial.println(".");
      }
       
      if(1)
      {
        // regular x/y mouse
        Mouse.move(x, y, 0);
      }
      else
      {
        // x scroll and y mouse
        scroll_accum += x;
        int scroll = scroll_accum / scroll_tick;
        scroll_accum %= scroll_tick;
        if(scroll != 0)
        {
          Serial.print("Scrolling by ");
          Serial.println(scroll);
          click();         
        }
        Mouse.move(0, y, scroll);        
      }
      
    }
}

void loop() 
{
  // Mouse movement is completely interrupt-driven.
 
  // Poll for button states 
  for(int i=0; i<buttonCount; i++)
  {
    int name = buttonNames[i];
    if (digitalRead(buttonPins[i]) == LOW)
    {
      if (!Mouse.isPressed(name))
      {
        if (serial_debug)
        {
          Serial.print("pressing mouse button ");
          Serial.println(name);
        }
        Mouse.press(name);
      }
    }
    else
    {
      if (Mouse.isPressed(name))
      {
        if (serial_debug)
        {
          Serial.print("releasing mouse button ");
          Serial.println(name);
        }
        Mouse.release(name);
      }
    }
  }
}


