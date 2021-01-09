#include <SPI.h>
#include <avr/pgmspace.h>

#if ARDUINO >= 10606
  // If we have support for pluggable HID/USB modules, make this device look like just a mouse.
  #include <HID.h>
  #include <Mouse.h>
#endif

#include "trackball.h"

#include "adns.h"

/*
  Pin assignments on Sparkfun Pro Micro:
  
  4 - left button
  5 - middle button
  6 - right button
  3 - piezo speaker +
  
  16 - SPI MOSI (MO)
  14 - SPI MISO (MI)
  15 - SPI SCLK (SC)
  18 - sensor 1 chip select (SS)
  19 - sensor 2 chip select (SS)
  
  VCC - sensor VI
  GND - sensor DG & AG
  
*/

byte initComplete = 0;

adns sensor_1;
adns sensor_2;

// Button state polling/tracking
const int buttonCount = 3;
const char buttonNames[3] = {MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
const int buttonPins[3] = {4, 5, 6};

const int piezo_pin = 3;

const int scroll_tick = 128;
int scroll_accum = 0;

void setup() 
{
  Mouse.begin();
  
#if SERIAL_DEBUG
  Serial.begin(9600);
  // Add a short delay so I can get the console open before things start happening.
  delay(2000);
#endif
    
  SPI.begin();
  
  sensor_1.init(18);
  sensor_2.init(19);
  
  sensor_1.set_cpi(2400);
  sensor_1.set_snap_angle(1);
  sensor_2.set_cpi(2400);
  sensor_2.set_snap_angle(1);

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
  delay(1);
  digitalWrite(piezo_pin, LOW);
}


void loop() 
{
  // Poll sensors for mouse movement
  {
      sensor_1.read_motion();
      sensor_2.read_motion();
      
      if (sensor_1.x != 0 || sensor_1.y != 0 || sensor_2.x != 0 || sensor_2.y != 0)
      {
          DebugLog(F("1x = "));
          DebugLog(sensor_1.x);
          DebugLog(F(", 1y = "));
          DebugLog(sensor_1.y);
          DebugLog(F(", 2x = "));
          DebugLog(sensor_2.x);
          DebugLog(F(", 2y = "));
          DebugLogln(sensor_2.y);
      }

      int x = 0, y = 0, scroll = 0;

      x = -sensor_2.x;
      y = sensor_1.x;
      
      // Figure out if we should scroll
      if(1)
      {
        if ((abs(sensor_1.y) > (abs(sensor_1.x) * 2)) && (abs(sensor_2.y) > (abs(sensor_2.x) * 2)))
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
        DebugLog(F("pressing mouse button "));
        DebugLogln(name);
        Mouse.press(name);
      }
    }
    else
    {
      if (Mouse.isPressed(name))
      {
        DebugLog(F("releasing mouse button "));
        DebugLogln(name);
        Mouse.release(name);
      }
    }
  }
}


