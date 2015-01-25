#include <SPI.h>
#include <avr/pgmspace.h>

#include "adns.h"

/*
  Pin assignments on Sparkfun Pro Micro:
  
  4 - left button
  5 - middle button
  6 - right button
  9 - piezo speaker +
  
  16 - SPI MOSI (MO)
  14 - SPI MISO (MI)
  15 - SPI SCLK (SC)
  18 - sensor 1 chip select (SS)
  18 - sensor 2 chip select (SS)
  
  VCC - sensor VI
  GND - sensor DG & AG
  
*/

byte initComplete = 0;
byte serial_debug = 0;

adns sensor_1;

// Button state polling/tracking
const int buttonCount = 3;
const char buttonNames[3] = {MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
const int buttonPins[3] = {4, 5, 6};

const int piezo_pin = 9;

const int scroll_tick = 128;
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
    
  SPI.begin();
  
  sensor_1.init(18);

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


void loop() 
{
  // Poll sensors for mouse movement
  {
      sensor_1.read_motion();
      int x = sensor_1.x;
      int y = sensor_1.y;
      int scroll = 0;
      
      if(0)
      {
        // testing: turn x into scroll
        scroll_accum += x;
        scroll = scroll_accum / scroll_tick;
        scroll_accum %= scroll_tick;
        x = 0;
      }
      
      if ((x != 0) || (y != 0) || (scroll != 0))
      {
        if (scroll != 0)
        {
          click();
        }
        Mouse.move(x, y, scroll);        
      }
  }
  
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


