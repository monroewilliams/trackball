#include <SPI.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "adns.h"

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_MOUSE()
};

// USB HID object
Adafruit_USBD_HID usb_hid;


/*
  Common pin assignments:
  standard SPI (MOSI, MISO, SCLK)
  VCC - sensor VI
  GND - sensor DG & AG
*/

#ifdef ARDUINO_AVR_PROMICRO8
  #include <avr/pgmspace.h>
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

#ifdef SEEED_XIAO_M0
  // Pin assignments on Seeeduino XIAO:
  // piezo speaker +
  #define PIN_PIEZO 5
  // Mouse button inputs
  #define PIN_BUTTON_LEFT 0 
  #define PIN_BUTTON_RIGHT 1 
  #define PIN_BUTTON_MIDDLE 2 
  // Select pins for sensors
  #define PIN_SENSOR_1_SELECT 7  
  #define PIN_SENSOR_2_SELECT 6  
#endif

adns sensor_1;
adns sensor_2;

// Button state polling/tracking
const char buttonNames[] = { MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE };
const int buttonPins[] = { PIN_BUTTON_LEFT, PIN_BUTTON_RIGHT,  PIN_BUTTON_MIDDLE };
const int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
char buttons;

const int scroll_tick = 128;
int scroll_accum = 0;

void setup() 
{
  // pinMode(LED_BUILTIN, OUTPUT);

  // Free up hardware serial pins for our use.
  Serial1.end();

  // TinyUSB Setup
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  // usb_hid.setStringDescriptor("MWTrackball");

  usb_hid.begin();

  // wait until device mounted
  while( !USBDevice.mounted() ) delay(1);


#if SERIAL_DEBUG
  Serial.begin(115200);

  // Wait for the serial port to be opened.
  while (!Serial);

  // Add a short delay so I can get the console open before things start happening.
  // delay(3000);
  Serial.println(F("Opened serial port"));
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
  
#ifdef PIN_PIEZO
  // Piezo output
  pinMode(PIN_PIEZO, OUTPUT);
#endif

#if SERIAL_DEBUG
  Serial.println(F("Initialization complete."));
  delay(100);
#endif

}

void click()
{
#ifdef PIN_PIEZO
  digitalWrite(PIN_PIEZO, HIGH);
  delay(1);
  digitalWrite(PIN_PIEZO, LOW);
#endif
}


void loop() 
{
  bool sendReport = false;
  bool sendWakeup = false;
  int x = 0, y = 0, scroll = 0;

  // Poll sensors for mouse movement
  if(1)
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
        sendReport = true;
      }
  }
  
  // Poll for button states 
  if(1)
  {
    for(int i=0; i<buttonCount; i++)
    {
      int name = buttonNames[i];
      if (digitalRead(buttonPins[i]) == LOW)
      {
        if (!(buttons && name))
        {
          DebugLog(F("pressing mouse button "));
          DebugLogln(name);
          buttons |= name;
          sendReport = true;
          sendWakeup = true;
        }
      }
      else
      {
        if (buttons && name)
        {
          DebugLog(F("releasing mouse button "));
          DebugLogln(name);
          buttons &= ~name;
          sendReport = true;
          sendWakeup = true;
        }
      }
    }
  }
  if (sendWakeup && USBDevice.suspended())
  {
    USBDevice.remoteWakeup();
  }
  if (sendReport)
  {
      usb_hid.mouseReport(0, buttons, x, y, scroll, 0);
  }

  delay(1);
}


