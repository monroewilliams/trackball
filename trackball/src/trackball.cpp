#include <SPI.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "adns.h"

#define USE_SCROLL_RESOLUTION_MULTIPLIER 1

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] =
{
#if !USE_SCROLL_RESOLUTION_MULTIPLIER
  TUD_HID_REPORT_DESC_MOUSE()
#else
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                   ,
  HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE     )                   ,
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                   ,
    /* Report ID if any */
    // __VA_ARGS__ 
    HID_USAGE      ( HID_USAGE_DESKTOP_POINTER )                   ,
    HID_COLLECTION ( HID_COLLECTION_PHYSICAL   )                   ,
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON  )                   ,
        HID_USAGE_MIN   ( 1                                      ) ,
        HID_USAGE_MAX   ( 5                                      ) ,
        HID_LOGICAL_MIN ( 0                                      ) ,
        HID_LOGICAL_MAX ( 1                                      ) ,
        /* Left, Right, Middle, Backward, Forward buttons */ 
        HID_REPORT_COUNT( 5                                      ) ,
        HID_REPORT_SIZE ( 1                                      ) ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,
        /* 3 bit padding */ 
        HID_REPORT_COUNT( 1                                      ) ,
        HID_REPORT_SIZE ( 3                                      ) ,
        HID_INPUT       ( HID_CONSTANT                           ) ,
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,
        /* X, Y position [-127, 127] */ 
        HID_USAGE       ( HID_USAGE_DESKTOP_X                    ) ,
        HID_USAGE       ( HID_USAGE_DESKTOP_Y                    ) ,
        HID_LOGICAL_MIN ( 0x81                                   ) ,
        HID_LOGICAL_MAX ( 0x7f                                   ) ,
        HID_REPORT_COUNT( 2                                      ) ,
        HID_REPORT_SIZE ( 8                                      ) ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,
    HID_COLLECTION_END                                             , 
    HID_COLLECTION ( HID_COLLECTION_LOGICAL   )                    ,
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,
        HID_USAGE       ( HID_USAGE_DESKTOP_RESOLUTION_MULTIPLIER) ,
        HID_LOGICAL_MIN ( 0                                      ) ,
        HID_LOGICAL_MAX ( 255                                    ) ,
        HID_PHYSICAL_MIN( 1                                      ) ,
        HID_PHYSICAL_MAX_N( 256, 2                               ) ,
        HID_REPORT_COUNT( 1                                      ) ,
        HID_REPORT_SIZE ( 8                                      ) ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,
        /* Veritcal wheel scroll [-127, 127] */ 
        HID_USAGE       ( HID_USAGE_DESKTOP_WHEEL                )  ,
        HID_LOGICAL_MIN ( 0x81                                   )  ,
        HID_LOGICAL_MAX ( 0x7f                                   )  ,
        HID_REPORT_COUNT( 1                                      )  ,
        HID_REPORT_SIZE ( 8                                      )  ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE )  ,
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_CONSUMER ), 
       /* Horizontal wheel scroll [-127, 127] */ 
        HID_USAGE_N     ( HID_USAGE_CONSUMER_AC_PAN, 2           ), 
        HID_LOGICAL_MIN ( 0x81                                   ), 
        HID_LOGICAL_MAX ( 0x7f                                   ), 
        HID_REPORT_COUNT( 1                                      ), 
        HID_REPORT_SIZE ( 8                                      ), 
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ), 
    HID_COLLECTION_END                                            , 
  HID_COLLECTION_END 
#endif
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
  // sensor_1.set_snap_angle(1);
  sensor_2.set_cpi(2400);
  // sensor_2.set_snap_angle(1);

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
  int x = 0, y = 0, scroll = 0, scroll_raw = 0;

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

      if(0)
      {
        // Original sensor arrangement (s1 at 180 and s2 at 270)
        x = -sensor_2.x;
        y = sensor_1.x;
      }
      else
      {
        // New sensor arrangement (sensors at 180 and 270 + 45)
        // Sensors are also tilted down by about 30 degrees, for easier mounting, but I don't _think_ that changes the numbers.
        
        // The y sensor reading is correct as-is.
        y = sensor_1.x;

        // The x sensor will be "contaminated" with a Y component, and also attenuated due to being off-axis.
        x = (
              - sensor_2.x                  // original amount
              - (y * (1.0 / sqrtf(2.0)))    // subtract out y
            ) * sqrtf(2.0);                 // and scale up to counter attenuation

      }

      // Figure out if we should scroll
      if ((abs(sensor_1.y) > (abs(sensor_1.x) * 2)) && (abs(sensor_2.y) > (abs(sensor_2.x) * 2)))
      {
        // Looks like we're scrolling more than not.  Take the average of the two sensors' y deltas as the scroll amount.
        scroll_raw = -((sensor_1.y + sensor_2.y) / 2);
        scroll_accum += scroll_raw;
        scroll = scroll_accum / scroll_tick;
        scroll_accum %= scroll_tick;
        // When we're scrolling, disable x/y movement
        x = 0;
        y = 0;
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
#if !USE_SCROLL_RESOLUTION_MULTIPLIER
      usb_hid.mouseReport(0, buttons, x, y, scroll, 0);
#else
    // With the resolution multiplier, we have deviated from the standard mouse report.
    // Roll our own here.
    typedef struct TU_ATTR_PACKED
    {
      uint8_t buttons;    /**< buttons mask for currently pressed buttons in the mouse. */
      int8_t  x;          /**< Current delta x movement of the mouse. */
      int8_t  y;          /**< Current delta y movement on the mouse. */
      int8_t  multiplier; /**< Mouse wheel resolution multiplier */
      int8_t  wheel;      /**< Current delta wheel movement on the mouse. */
      int8_t  pan;        // using AC Pan
    } report_t;

    report_t report =
    {
      .buttons = buttons,
      .x       = int8_t(x),
      .y       = int8_t(y),
      .multiplier = int8_t(scroll_tick),
      .wheel   = int8_t(scroll_raw),
      .pan     = 0
    };

    (void)tud_hid_report(0, &report, sizeof(report));
#endif
  }

  delay(1);
}


