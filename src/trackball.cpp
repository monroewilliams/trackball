#include <SPI.h>
#include <math.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "Vector.h"
#include "adns.h"

////////////////////////////////////////
// Various config options

// Enable serial port debugging output
#define SERIAL_DEBUG 0

// Use a custom HID descriptor instead of TUD_HID_REPORT_DESC_MOUSE()
#define USE_CUSTOM_HID_DESCRIPTOR 0

// Use HID_USAGE_DESKTOP_RESOLUTION_MULTIPLIER to transmit higher-fidelity scroll reports.
// Only used if USE_CUSTOM_HID_DESCRIPTOR is also set to 1.
#define USE_SCROLL_RESOLUTION_MULTIPLIER 0

// Use 16 bit fields for the X/Y deltas in the report.
// Only used if USE_CUSTOM_HID_DESCRIPTOR is also set to 1.
#define USE_16_BIT_DELTAS 0

// Look for an SSD1327 display on the i2c bus and use it to display the images from the sensors.
// The display I'm using is one of these: https://www.adafruit.com/product/4741
#define SENSOR_DISPLAY 0

// If this is 0, don't scale the luminance of the sensor data.
// If it's 1, scale using integer divide.
// If it's 2, scale using floating point.
// Only used if SENSOR_DISPLAY is also set to 1
#define SCALE_SENSOR_LUMINANCE 2

//
///////////////////////////////////////

#if SENSOR_DISPLAY
  #include <Adafruit_SSD1327.h>
#endif

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
#if USE_16_BIT_DELTAS
        HID_LOGICAL_MIN_N ( SHRT_MIN, 2                          ) ,
        HID_LOGICAL_MAX_N ( SHRT_MAX, 2                          ) ,
        HID_REPORT_COUNT( 2                                      ) ,
        HID_REPORT_SIZE ( 16                                     ) ,
#else
        HID_LOGICAL_MIN ( 0x81                                   ) ,
        HID_LOGICAL_MAX ( 0x7f                                   ) ,
        HID_REPORT_COUNT( 2                                      ) ,
        HID_REPORT_SIZE ( 8                                      ) ,
#endif
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


#if SENSOR_DISPLAY
Adafruit_SSD1327 display(128, 128, &Wire, -1, 1000000UL);
const int display_address = SSD1327_I2C_ADDRESS;
bool sensor_display_mode = false;
bool display_ready = false;

void reset_display()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(0,0);
  // Leave room for live updates
  display.print("\n\n\n\n");
  if (sensor_display_mode) {
    display.print(F("Press all 3 buttons\ntogether to return\nto trackball mode."));
    display.setContrast(0x7f);
  } else {    
    display.print(F("Press all 3 buttons\ntogether to view \nsensor data."));
    display.setContrast(0x1f);
  }
  display.display();
}

void draw_sensor_pixels(int x, int y, uint8_t *pixels)
{
#if SCALE_SENSOR_LUMINANCE
  uint8_t lut[256];
  // Scale the data to show maxumum contrast using the 16 gray levels available.
  // Scan the data and find the min and max pixel values
  uint8_t min = 255;
  uint8_t max = 0;
  for (int i = 0; i < 900; i++)
  {
    uint8_t cur = pixels[i];
    if (cur < min) min = cur;
    if (cur > max) max = cur;
  }
  // Scale the pixels so that the darkest gets value 0 and the brightest gets 15
  int range = max - min;
  uint8_t offset = min;

#if SCALE_SENSOR_LUMINANCE == 2
  // use a floating point scale value
  float scale = 1.0 / ((range > 0)?((range * 16) / 255.0):(16));
  for(int i = min; i <=max; i++){
    uint8_t color = (i - offset) * scale;
    lut[i] = color | (color << 4);
  }
#else
  // use integer divide
  int divisor = (range > 0)?((range * 16)/ 255):16;
  for(int i = min; i <=max; i++){
    uint8_t color = (i - offset) / divisor;
    if (color > 15) color = 15;
    lut[i] = color | (color << 4);
  }
#endif

#endif

  // This blit converts 8 bit to 4 bit, and also expands the data 2x in x and y.

  // Touch the pixels at the corners of this blit to update the dirty rect
  display.drawPixel(x, y, 0);
  display.drawPixel(x + 59, y + 59, 0);

  uint8_t *buffer = display.getBuffer();
  const int rowbytes = 64;
  // Assumptions:
  // X is even
  // the blit does not need to be clipped
  buffer += (rowbytes * y) + (x >> 1);

  int i = 0;
  for(int iy = 0; iy < 30; iy++)
  {
    uint8_t *dst = buffer;
    for(int ix = 0; ix < 30; ix++)
    {
      uint16_t color = pixels[i];
  #if SCALE_SENSOR_LUMINANCE
      color = lut[color];
  #else
      color >>= 4;
      color |= color << 4;
  #endif
      // write two pixels at once, in two lines
      dst[0] = color;
      dst[rowbytes] = color;
      dst++;
      i++;
    }
    // move the buffer pointer down two rows
    buffer += rowbytes << 1;
  }

}

void display_sensors()
{
  uint8_t pixels[900];
  unsigned long read1 = micros();
  s1.read_image(pixels);
  unsigned long render1 = micros();
  draw_sensor_pixels(0, 64, pixels);
  unsigned long read2 = micros();
  s2.read_image(pixels);
  unsigned long render2 = micros();
  draw_sensor_pixels(64, 64, pixels);
  unsigned long xfer = micros();
  display.display();
  unsigned long end = micros();

  debugLogger.print(F("read took "));
  debugLogger.print((render1 - read1) + (render2 - read2));
  debugLogger.print(F(", render took "));
  debugLogger.print((read2 - render1) + (xfer - render2));
  debugLogger.print(F(", xfer took "));
  debugLogger.println(end - xfer);
}

void set_sensor_display(bool enable)
{
  if (!sensor_display_mode && enable) {
    sensor_display_mode = true;
    reset_display();
    s1.begin_image_capture();
    s2.begin_image_capture();
  } else if (sensor_display_mode && !enable) {
    sensor_display_mode = false;
    reset_display();
    s1.end_image_capture();
    s2.end_image_capture();
  }

}

void disable_sensor_display()
{
  sensor_display_mode = false;
  s1.end_image_capture();
  s2.end_image_capture();
}

bool i2c_probe_bus()
{
  // Check to see whether it looks like there's anything connected to the SDA/SCL pins.
  // This is needed because calling Wire.begin() without anything conneted seems to break things.
  // Specifically, it returns false positives and sometimes hangs the device.

  // If i2c is connected, both pins should have pull-up resistors, which means they will read high.
  pinMode(PIN_WIRE_SDA, INPUT);
  pinMode(PIN_WIRE_SCL, INPUT);
  return (digitalRead(PIN_WIRE_SDA) == HIGH) && (digitalRead(PIN_WIRE_SDA) == HIGH);
}

bool i2c_probe_device(byte address)
{
  // Probe the specified address to see if a device seems to be present.
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

#endif

DebugLogger debugLogger;

size_t DebugLogger::write(uint8_t c)
{
  size_t result = 0;
  // This sort of works, but doesn't scroll and is way too slow to be useful.
// #if SENSOR_DISPLAY
//   if (display_ready)
//   {
//     result = display.write(c);
//     display.display();
//   }
// #endif
#if SERIAL_DEBUG
  if (tud_cdc_connected())
  {
    result = Serial.write(c);
  }
#endif

  return result;
}
size_t DebugLogger::write(const uint8_t *buffer, size_t size)
{
  size_t result = 0;
  // This sort of works, but doesn't scroll and is way too slow to be useful.
// #if SENSOR_DISPLAY
//   if (display_ready)
//   {
//     result = display.write(buffer, size);
//     display.display();
//   }
// #endif

#if SERIAL_DEBUG
  if (tud_cdc_connected())
  {
    result = Serial.write(buffer, size);
  }
#endif

  return result;
}

bool DebugLogger::enabled()
{
  bool result = false;
  // This sort of works, but doesn't scroll and is way too slow to be useful.
// #if SENSOR_DISPLAY
//   if (display_ready)
//   {
//     result = true;
//   }
// #endif
#if SERIAL_DEBUG
  if (tud_cdc_connected())
  {
    result = true;
  }
#endif
  return result;
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
  // while (!Serial);

  // Add a short delay so I can get the console open before things start happening.
  delay(3000);

  debugLogger.println(F("Opened serial port"));
#endif

#if SENSOR_DISPLAY
  if (!i2c_probe_bus())
  {
    debugLogger.println(F("i2c bus appears disconnected"));
  }
  else
  {
    debugLogger.println(F("i2c bus seems sane"));
    Wire.begin();
    if (!i2c_probe_device(display_address))
    {
      debugLogger.println(F("Display not found"));
    }
    else
    {
      debugLogger.println(F("Display found"));
      // Something responded at the correct address. Assume it's the display.
      if (!display.begin(display_address)) 
      {
        debugLogger.println(F("Display init failed"));
      }
      else
      {
        display_ready = true;
        debugLogger.println(F("Display initialized"));

        reset_display();
      }
    }
  }
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

debugLogger.println(F("Initialization complete."));

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
  // Time taken by the last loop, saved so we can display it during the next loop.
  static unsigned long loop_time = 0;
  bool sendReport = false;
  bool sendWakeup = false;
  Vector delta;
  int scroll = 0;

#if SENSOR_DISPLAY
  if(sensor_display_mode)
  {
    display_sensors();
  }
  else
#endif
  {
    // Poll sensors for mouse movement

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
      // debugLogger.print(F("v1 = "));
      // debugLogger.print(v1);
      // debugLogger.print(F(", v2 = "));
      // debugLogger.print(v2);
      // debugLogger.print(F(", delta = "));
      // debugLogger.print(delta);
      // debugLogger.println("");

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
        // debugLogger.print("Calculated scroll = ");
        // debugLogger.print(scroll);
        // debugLogger.print(", scroll_accum =  ");
        // debugLogger.println(scroll_accum);
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
          // debugLogger.print(F("pressing mouse button "));
          // debugLogger.println(name);
          buttons |= name;
          sendReport = true;
          sendWakeup = true;
        }
      }
      else
      {
        if (buttons & name)
        {
          // debugLogger.print(F("releasing mouse button "));
          // debugLogger.println(name);
          buttons &= ~name;
          sendReport = true;
          sendWakeup = true;
        }
      }
    }
#if SENSOR_DISPLAY
    // Only if the display was found on startup...
    if (display_ready)
    {
      // Check for the toggle condition
      static char prevButtons = 0;
      if (buttons == 0x07 && prevButtons != 0x07)
      {
        set_sensor_display(!sensor_display_mode);
      }
      prevButtons = buttons;
    }
#endif

  }
  if (sendWakeup && USBDevice.suspended())
  {
    USBDevice.remoteWakeup();
  }
  if (sendReport)
  {
    #define CLAMP(val, min, max) (val > max)?max:((val < min)?min:val)
#if !USE_CUSTOM_HID_DESCRIPTOR
      // if (scroll != 0)
      // {
      //   debugLogger.print("reporting wheel = ");
      //   debugLogger.println(scroll);
      // }
      usb_hid.mouseReport(
        0, 
        buttons, 
        CLAMP(delta.x, SCHAR_MIN, SCHAR_MAX),
        CLAMP(delta.y, SCHAR_MIN, SCHAR_MAX),
        CLAMP(scroll, SCHAR_MIN, SCHAR_MAX),
        0);
#else
#if USE_16_BIT_DELTAS
    typedef int16_t delta_t;
    const int delta_min = SHRT_MIN;
    const int delta_max = SHRT_MAX;
#else
    typedef int8_t delta_t;
    const int delta_min = SCHAR_MIN;
    const int delta_max = SCHAR_MAX;
#endif

    // With the resolution multiplier, we have deviated from the standard mouse report.
    // Roll our own here.
    typedef struct TU_ATTR_PACKED
    {
      uint8_t buttons;    /**< buttons mask for currently pressed buttons in the mouse. */
      delta_t  x;         /**< Current delta x movement of the mouse. */
      delta_t  y;         /**< Current delta y movement on the mouse. */
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      uint8_t  multiplier; /**< Mouse wheel resolution multiplier */
#endif
      int8_t  wheel;      /**< Current delta wheel movement on the mouse. */
      int8_t  pan;        // using AC Pan
    } report_t;

    report_t report =
    {
      .buttons = buttons,
      .x       = delta_t(CLAMP(delta.x, delta_min, delta_max)),
      .y       = delta_t(CLAMP(delta.y, delta_min, delta_max)),
#if USE_SCROLL_RESOLUTION_MULTIPLIER
      .multiplier = uint8_t(scroll_tick - 1),
      .wheel   = int8_t(CLAMP(delta.z, SCHAR_MIN, SCHAR_MAX)),
#else
      .wheel   = int8_t(CLAMP(scroll, SCHAR_MIN, SCHAR_MAX)),
#endif
      .pan     = 0
    };

//     if (report.wheel != 0)
//     {
//       debugLogger.print("reporting wheel = ");
//       debugLogger.print(report.wheel);
// #if USE_SCROLL_RESOLUTION_MULTIPLIER
//       debugLogger.print(", multiplier = ");
//       debugLogger.print(report.multiplier);
// #endif
//       debugLogger.println("");
//     }

    (void)tud_hid_report(0, &report, sizeof(report));
#endif
  }
#if SENSOR_DISPLAY  
  if (display_ready)
  {
    ////////
    // This may be useful for debugging, but it brings us down to ~30hz.
  //   display.fillRect(0, 0, 128, 16, 0);
  //   display.setCursor(0,0);
  //   display.println(delta);
  //   display.print(F("loop time "));
  //   display.print(loop_time / 1000);
  //   display.print(F("ms"));
  //   display.display();
    ////////
    // Trying to diagnose an issue with kvm switching. This lets me see if the device resets on switch.
    // static unsigned long last_uptime_seconds = 0;
    // unsigned long uptime_seconds = millis() / 1000;
    // if (uptime_seconds != last_uptime_seconds)
    // {
    //   last_uptime_seconds = uptime_seconds;
    //   // Default font is 6x8
    //   display.fillRect(0, 0, 8 * 6, 1 * 8, 0);
    //   display.setCursor(0,0);
    //   display.printf("%02d:%02d:%02d", uptime_seconds / 3600, (uptime_seconds / 60) % 60, (uptime_seconds) % 60);
    //   display.display();
    // }
  }
#endif

  // Delay to keep the loop time right around report_microseconds
  loop_time = micros() - loop_start_time;
  if (loop_time < report_microseconds)
  {
    delayMicroseconds(report_microseconds - loop_time);
  }
    
}


