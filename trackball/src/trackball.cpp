#include <SPI.h>
#include <avr/pgmspace.h>

#include <Mouse.h>

#include "trackball.h"

#include "adns.h"


/*
  Common pin assignments:
  standard SPI (MOSI, MISO, SCLK)
  VCC - sensor VI
  GND - sensor DG & AG
*/

#ifdef ARDUINO_AVR_PROMICRO8
  // Pin assignments on Sparkfun Pro Micro:
  // piezo speaker +
  #define PIN_PIEZO 3
  // Mouse button inputs
  #define PIN_BUTTON_LEFT 4 
  #define PIN_BUTTON_MIDDLE 5 
  #define PIN_BUTTON_RIGHT 6 
  // Select pins for sensors
  #define PIN_SENSOR_1_SELECT 18  
  #define PIN_SENSOR_2_SELECT 19  
#endif  


byte initComplete = 0;

adns sensor_1;
adns sensor_2;

// Button state polling/tracking
const int buttonCount = 3;
const char buttonNames[3] = {MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT};
const int buttonPins[3] = {PIN_BUTTON_LEFT, PIN_BUTTON_MIDDLE, PIN_BUTTON_RIGHT};

const int scroll_tick = 128;
int scroll_accum = 0;

void setup() 
{
  Mouse.begin();
  
#if SERIAL_DEBUG
  Serial.begin(9600);
  // Add a short delay so I can get the console open before things start happening.
  delay(3000);
#endif
    
  SPI.begin();
  
  sensor_1.init(PIN_SENSOR_1_SELECT);
  sensor_2.init(PIN_SENSOR_2_SELECT);
  
  sensor_1.set_cpi(2400);
  sensor_1.set_snap_angle(1);
  sensor_2.set_cpi(2400);
  sensor_2.set_snap_angle(1);

  for(int i=0; i<buttonCount; i++)
  {
      pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Piezo output
  pinMode(PIN_PIEZO, OUTPUT);
  
  initComplete = 1;
}

void click()
{
  digitalWrite(PIN_PIEZO, HIGH);
  delay(1);
  digitalWrite(PIN_PIEZO, LOW);
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


