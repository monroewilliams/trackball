#include <SPI.h>
#include <math.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "adns.h"

#define USE_SCROLL_RESOLUTION_MULTIPLIER 0
#define USE_CUSTOM_HID_DESCRIPTOR 1

#define CUMULATIVE_MOTION_DEBUG 0
// #define CUMULATIVE_MOTION_DEBUG SERIAL_DEBUG

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
        HID_LOGICAL_MIN ( 0x81                                   ) ,
        HID_LOGICAL_MAX ( 0x7f                                   ) ,
        HID_REPORT_COUNT( 2                                      ) ,
        HID_REPORT_SIZE ( 8                                      ) ,
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


// Button state polling/tracking
const char buttonNames[] = { MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE };
const int buttonPins[] = { PIN_BUTTON_LEFT, PIN_BUTTON_RIGHT,  PIN_BUTTON_MIDDLE };
const int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
char buttons;

// sensors
adns sensor_1;
adns sensor_2;
#define SENSOR_CPI 2400

// sensor location
// azimuth = degrees clockwise from 12 o'clock (directly away from the user)
// elevation = degrees down from horizontal
// inverted = sensor is rotated 180 degrees (so the wires come out on the top instead of the bottom)
#define SENSOR_1_AZIMUTH 180
#define SENSOR_1_ELEVATION 30
#define SENSOR_1_INVERTED 1
#define SENSOR_2_AZIMUTH (270 + 45)
#define SENSOR_2_ELEVATION 30
#define SENSOR_2_INVERTED 1

Vector sensor_1_weights;
Vector sensor_2_weights;

#if CUMULATIVE_MOTION_DEBUG
Vector cumulative_motion;
#endif

// scrolling
const int scroll_tick = 64;
float scroll_accum = 0;

void calculate_weights(float azimuth, float elevation, Vector &out)
{
    // This currently ignores the elevation of the X and Y sensors.
    // This should work fine for now, but doesn't give a completely accurate picture of the ball rotation.
    out.x = sin(DEG_TO_RAD * azimuth);
    // For correct directionality, we actually want y to be from 6 o'clock. Adjust that here.
    out.y = cos(DEG_TO_RAD * (azimuth + 180));
    out.z = cos(DEG_TO_RAD * elevation);
}

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
    
  SPI.begin();
  
  sensor_1.init(PIN_SENSOR_1_SELECT);
  sensor_2.init(PIN_SENSOR_2_SELECT);
  
  sensor_1.set_cpi(SENSOR_CPI);
  sensor_2.set_cpi(SENSOR_CPI);
  
  for(int i=0; i<buttonCount; i++)
  {
      pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Calculate the X, Y, and Z weights of the sensor angles.
calculate_weights(SENSOR_1_AZIMUTH, SENSOR_1_ELEVATION, sensor_1_weights);
calculate_weights(SENSOR_2_AZIMUTH, SENSOR_2_ELEVATION, sensor_2_weights);

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
  Vector delta;
  int scroll = 0;

  // Poll sensors for mouse movement
  if(1)
  {
    sensor_1.read_motion();
    sensor_2.read_motion();
    
    // Cheat here by modifying the read values in the sensor objects.
    if (SENSOR_1_INVERTED)
    {
      sensor_1.x = -sensor_1.x;
      sensor_1.y = -sensor_1.y;
    }
    if (SENSOR_2_INVERTED)
    {
      sensor_2.x = -sensor_2.x;
      sensor_2.y = -sensor_2.y;
    }

    if (sensor_1.x != 0 || sensor_1.y != 0 || sensor_2.x != 0 || sensor_2.y != 0)
    {
      // Something moved.

      // DebugLog(F("1x = "));
      // DebugLog(sensor_1.x);
      // DebugLog(F(", 1y = "));
      // DebugLog(sensor_1.y);
      // DebugLog(F(", 2x = "));
      // DebugLog(sensor_2.x);
      // DebugLog(F(", 2y = "));
      // DebugLogln(sensor_2.y);

      // This isn't quite correct (x movement is polluted with y). I think I've oversimplified the math.
      if(CUMULATIVE_MOTION_DEBUG)
      {
        // For each component, use the sensor that would have the highest contribution
        // (and therefore the best resolution) for that component.
        delta.x = fabs(sensor_1_weights.x) > fabs(sensor_2_weights.x) ? 
          sensor_1.x * sensor_1_weights.x :
          sensor_2.x * sensor_2_weights.x ;
        delta.y = fabs(sensor_1_weights.y) > fabs(sensor_2_weights.y) ? 
          sensor_1.x * sensor_1_weights.y :
          sensor_2.x * sensor_2_weights.y ;
        delta.z = fabs(sensor_1_weights.z) > fabs(sensor_2_weights.z) ? 
          sensor_1.y * sensor_1_weights.z :
          sensor_2.y * sensor_2_weights.z ;

#if CUMULATIVE_MOTION_DEBUG
        cumulative_motion += delta;

        DebugLog(F("cumulative = "));
        cumulative_motion.print();
        DebugLogln("");
#endif
      }

      if(0)
      {
        // Original sensor arrangement (s1 at 180 and s2 at 270)
        delta.x = -sensor_2.x;
        delta.y = sensor_1.x;

        // Average both sensors' rotation-aligned contribution.
        delta.z = -((sensor_1.y + sensor_2.y) / 2);
      }
      else
      {
        // New sensor arrangement (sensors at 180 and 270 + 45)
        // Sensors are also tilted down below horizontal by about 30 degrees, for easier mounting, but I don't _think_ that changes the numbers.
        
        // The y sensor reading is correct as-is.
        delta.y = sensor_1.x;

        // The x sensor will be "contaminated" with a Y component, and also attenuated due to being off-axis.
        // This calculation isn't quite correct, but it's close enough.
        delta.x = (
              - sensor_2.x                        // original amount
              - (delta.y * (1.0 / sqrtf(2.0)))    // subtract out y
            ) * sqrtf(2.0);                       // and scale up to counter attenuation

        // Average both sensors' rotation-aligned contribution.
        delta.z = -((sensor_1.y + sensor_2.y) / 2);
      }

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

#if CUMULATIVE_MOTION_DEBUG
          // HACK: reset cumulative motion on any button press
          cumulative_motion = Vector();
#endif
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
      int8_t  x;          /**< Current delta x movement of the mouse. */
      int8_t  y;          /**< Current delta y movement on the mouse. */
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      uint8_t  multiplier; /**< Mouse wheel resolution multiplier */
#endif
      int8_t  wheel;      /**< Current delta wheel movement on the mouse. */
      int8_t  pan;        // using AC Pan
    } report_t;

    report_t report =
    {
      .buttons = buttons,
      .x       = int8_t(delta.x),
      .y       = int8_t(delta.y),
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

  // 8ms puts our updates around 120hz, which should be sufficient.
  delay(8);
}


