#include <SPI.h>
#include <math.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "Vector.h"
#include "adns.h"

#define USE_SCROLL_RESOLUTION_MULTIPLIER 0
#define USE_CUSTOM_HID_DESCRIPTOR 1

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] =
{
#if !USE_CUSTOM_HID_DESCRIPTOR
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
        HID_LOGICAL_MIN_N ( SHRT_MIN, 2                          ) ,
        HID_LOGICAL_MAX_N ( SHRT_MAX, 2                          ) ,
        HID_REPORT_COUNT( 2                                      ) ,
        HID_REPORT_SIZE ( 16                                     ) ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,
    HID_COLLECTION_END                                             , 
    HID_COLLECTION ( HID_COLLECTION_LOGICAL   )                    ,
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,
        HID_USAGE       ( HID_USAGE_DESKTOP_RESOLUTION_MULTIPLIER) ,
        HID_LOGICAL_MIN ( 0                                      ) ,
        HID_LOGICAL_MAX ( 255                                    ) ,
        HID_PHYSICAL_MIN( 1                                      ) ,
        HID_PHYSICAL_MAX_N( 256, 2                               ) ,
        HID_REPORT_COUNT( 1                                      ) ,
        HID_REPORT_SIZE ( 8                                      ) ,
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,
#endif
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

#if defined(SEEED_XIAO_M0) || defined(ARDUINO_QTPY_M0)
  // Pin assignments on Seeeduino XIAO/Adafruit QT Py:
  // piezo speaker +
  #define PIN_PIEZO 3
  // Mouse button inputs
  #define PIN_BUTTON_LEFT 0 
  #define PIN_BUTTON_RIGHT 1 
  #define PIN_BUTTON_MIDDLE 2 
  // Select pins for sensors
  #define PIN_SENSOR_1_SELECT 7  
  #define PIN_SENSOR_2_SELECT 6  
#endif

#if defined(PIN_NEOPIXEL)
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
#endif

// Button state polling/tracking
const char buttonNames[] = { MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE };
const int buttonPins[] = { PIN_BUTTON_LEFT, PIN_BUTTON_RIGHT,  PIN_BUTTON_MIDDLE };
const int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
char buttons;

// This is the cpi we want to report to the host.
const int reported_cpi = 2400;
// and the frequency at which we want to poll/report.
const int report_Hz = 120;
const int report_microseconds = 1000000 / report_Hz;

// The coordinate system here is a bit wonky -- in the final HID report, +X points right and +Y points "down" (towards the user),
// and the direction of Z is arbitrary since we're translating it to scrollwheel ticks.
// We're also conflating linear motion on the X/Y plane of each sensor tangent to the ball with rotation of the ball in X/Y/Z

// sensor locations:
// S#A = azimuth = degrees clockwise from 12 o'clock (directly away from the user)
// S#E = elevation = degrees down from horizontal
#define S1A 180
#define S1E 30
#define S2A (270 + 45)
#define S2E 30

// TODO: make a transform out of the azimuth/elevation constants.

// The sensor transform
// Note that the sensor orientation I've been using all along is actually "sideways", for mechanical reasons
// i.e. the X axis of the sensor points up/down, and the Y axis points parallel to the desk.
// This transform takes that into account.
float st[3][4] = 
{
#if 1
  // This is the "hack" transform I've been using for the new sensor location 
  // (s1 at 180, s2 at  270 + 45, both at 30 degrees elevation)
  {  1,        0,        sqrtf(2.0),  0   },  // X is s2.x, scaled up a bit, with s1.x subtracted to compensate for s2 being off-axis
  { -1,        0,        0,           0   },  // Y is s1.x
  {  0,        0.5,      0,           0.5 }   // Z is the average of the two sensors' y components.
#else
  // This was the transform for the original sensor location (s1 at 180 and s2 at 270, at zero elevation)
  // The sensors were also inverted here (i.e. the part where the wires attach was at the bottom instead of the top)
  {  0,        0,       -1,     0   },    // X is the inverse of the direct x reading of s2
  {  1,        0,        0,     0   },    // Y is the direct x reading of s1
  {  0,       -0.5,      0,    -0.5  }    // Z is the average of the two sensors' y components.
#endif
};

// sensor hardware abstraction
adns s1(PIN_SENSOR_1_SELECT, reported_cpi);
adns s2(PIN_SENSOR_2_SELECT, reported_cpi);

// scrolling
const int scroll_tick = 64;
float scroll_accum = 0;

void setup() 
{
  // pinMode(LED_BUILTIN, OUTPUT);

  // Free up hardware serial pins for our use.
  Serial1.end();

  // TinyUSB Setup
  USBDevice.setProductDescriptor("MWTrackball");
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));

  usb_hid.begin();

  // wait until device mounted
  while( !USBDevice.mounted() ) delay(1);

#if SERIAL_DEBUG
  Serial.begin(115200);

  // Wait for the serial port to be opened.
  // NOTE: this will block forever, which can be inconvenient if you're trying to use the device.
  while (!Serial);

  // Add a short delay so I can get the console open before things start happening.
  // delay(1000);

  Serial.println(F("Opened serial port"));
#endif

#if defined(PIN_NEOPIXEL)
  pixel.begin();  // initialize the pixel

  // and light it up
  pixel.setPixelColor(0, 128, 0, 255);
  pixel.show();
#endif

  SPI.begin();
  
  s1.init();
  s2.init();
    
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
  // This is MUCH louder than just toggling the high/low once with digitalWrite().
  tone(PIN_PIEZO, 1500, 5);
#endif
}

void loop() 
{
  unsigned long loop_start_time = micros();
  bool sendReport = false;
  bool sendWakeup = false;
  Vector delta;
  int scroll = 0;

  // Poll sensors for mouse movement
  if(1)
  {
    // v1 and v2 contain the floating point x/y motion values from each sensor, 
    // scaled from the sensor's CPI to units of reported_cpi.
    Vector v1 = s1.motion();
    Vector v2 = s2.motion();
    
    if (v1.x != 0 || v1.y != 0 || v2.x != 0 || v2.y != 0)
    {
      // The sensor reported movement.

      // multiply the sensor transform matrix by the vector of [v1.x, v1.y, v2.x, v2.y]
      delta = Vector(
        st[0][0] * v1.x + st[0][1] * v1.y + st[0][2] * v2.x + st[0][3] * v2.y, 
        st[1][0] * v1.x + st[1][1] * v1.y + st[1][2] * v2.x + st[1][3] * v2.y, 
        st[2][0] * v1.x + st[2][1] * v1.y + st[2][2] * v2.x + st[2][3] * v2.y
      );

      ////// This is probably only useful when actively debugging sensor readings or the transform matrix.
      ////// Otherwise it gets very spammy.
      // DebugLog(F("v1 = "));
      // DebugLog(v1);
      // DebugLog(F(", v2 = "));
      // DebugLog(v2);
      // DebugLog(F(", delta = "));
      // DebugLog(delta);
      // DebugLogln("");

      // Figure out if we should scroll
      if ((fabs(delta.z) > (fabs(delta.x) * 2)) && (fabs(delta.z) > (fabs(delta.y) * 2)))
      {
        // Looks like we're scrolling more than not.
        scroll_accum += delta.z;
        scroll = scroll_accum / scroll_tick;
        scroll_accum -= scroll * scroll_tick;

        // When we're scrolling, disable x/y movement
        delta.x = 0;
        delta.y = 0;

        DebugLog("Calculated scroll = ");
        DebugLog(scroll);
        DebugLog(", scroll_accum =  ");
        DebugLogln(scroll_accum);
      }
      else
      {
        delta.z = 0;
      }

      if (scroll != 0)
      {
          click();
      }
      if ((delta.x != 0) || (delta.y != 0) || 
#if USE_SCROLL_RESOLUTION_MULTIPLIER
        (delta.z != 0)
#else
        (scroll != 0)
#endif
      )
      {
        sendReport = true;
      }
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
        if (!(buttons & name))
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
        if (buttons & name)
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
#if !USE_CUSTOM_HID_DESCRIPTOR
      if (scroll != 0)
      {
        DebugLog("reporting wheel = ");
        DebugLogln(scroll);
      }
      usb_hid.mouseReport(0, buttons, delta.x, delta.y, scroll, 0);
#else
    // With the resolution multiplier, we have deviated from the standard mouse report.
    // Roll our own here.
    typedef struct TU_ATTR_PACKED
    {
      uint8_t buttons;    /**< buttons mask for currently pressed buttons in the mouse. */
      int16_t  x;          /**< Current delta x movement of the mouse. */
      int16_t  y;          /**< Current delta y movement on the mouse. */
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      uint8_t  multiplier; /**< Mouse wheel resolution multiplier */
#endif
      int8_t  wheel;      /**< Current delta wheel movement on the mouse. */
      int8_t  pan;        // using AC Pan
    } report_t;

    report_t report =
    {
      .buttons = buttons,
      .x       = int16_t(delta.x),
      .y       = int16_t(delta.y),
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      .multiplier = uint8_t(scroll_tick - 1),
      .wheel   = int8_t(delta.z),
#else
      .wheel   = int8_t(scroll),
#endif
      .pan     = 0
    };

    if (report.wheel != 0)
    {
      DebugLog("reporting wheel = ");
      DebugLog(report.wheel);
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      DebugLog(", multiplier = ");
      DebugLog(report.multiplier);
#endif
      DebugLogln("");
    }

    (void)tud_hid_report(0, &report, sizeof(report));
#endif
  }

  // Delay to keep the loop time right around report_microseconds
  unsigned long time_elapsed = micros() - loop_start_time;
  if (time_elapsed < report_microseconds)
  {
    delayMicroseconds(report_microseconds - time_elapsed);
  }
    
}


