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
  19 - sensor 2 chip select (SS)
  
  VCC - sensor VI
  GND - sensor DG & AG
  
*/

byte initComplete = 0;
byte serial_debug = 0;

adns sensor_1;
adns sensor_2;

// Button state polling/tracking
const int buttonCount = 3;
const char buttonNames[3] = {MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
const int buttonPins[3] = {4, 5, 6};

const int piezo_pin = 9;

const int scroll_tick = 1024;
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
  sensor_2.init(19);

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
  unsigned long loop_start = millis();
  
  // Poll sensors for mouse movement
  {
      sensor_1.read_motion();
      sensor_2.read_motion();
      
      int x = 0, y = 0, scroll = 0;

      x = -sensor_2.x;
      y = sensor_1.x;
      
      // Figure out if we should scroll
      if(1)
      {
        if ((abs(sensor_1.y) > abs(sensor_1.x)) && (abs(sensor_2.y) > abs(sensor_2.x)))
        {
          // Looks like we're scrolling more than not.  Take the average of the two sensors' y deltas as the scroll amount.
          scroll_accum += -((sensor_1.y + sensor_2.y) / 2);
          scroll = scroll_accum / scroll_tick;
          scroll_accum %= scroll_tick;
          
          // When we're scrolling, disable x/y movement
          x = 0;
          y = 0;
        }
      }
      
      if (0)
      {
          Serial.print("1x = ");
          Serial.print(sensor_1.x);
          Serial.print(", 1y = ");
          Serial.print(sensor_1.y);
          Serial.print(", 2x = ");
          Serial.print(sensor_2.x);
          Serial.print(", 2y = ");
          Serial.println(sensor_2.y);
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


