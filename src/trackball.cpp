#include <SPI.h>
#include <math.h>

#include "Adafruit_TinyUSB.h"
#include "trackball.h"
#include "Vector.h"
#include "adns.h"

#if defined(PIN_NEOPIXEL)
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
#endif

////////////////////////////////////////
// Various config options

// This is the cpi we will report to the host. It's independent of the sensor's physical CPI.
const int reported_cpi = 800;

// This is the frequency at which we poll sensors/buttons and send HID reports.
const int report_Hz = 120;

// Use a custom HID descriptor instead of TUD_HID_REPORT_DESC_MOUSE()
#define USE_CUSTOM_HID_DESCRIPTOR 0

// Use HID_USAGE_DESKTOP_RESOLUTION_MULTIPLIER to transmit higher-fidelity scroll reports.
// Only used if USE_CUSTOM_HID_DESCRIPTOR is also set to 1.
#define USE_SCROLL_RESOLUTION_MULTIPLIER 0

// Use 16 bit fields for the X/Y deltas in the report
// Only used if USE_CUSTOM_HID_DESCRIPTOR is also set to 1.
#define USE_16_BIT_DELTAS 1

// Enable serial port debugging output
// #define SERIAL_DEBUG

// These give the option to connect a display and toggle between regular operation and displaying the images from the sensors.
// 0 - no display
// 1 - SSD1327 display on the i2c bus  ( https://www.adafruit.com/product/4741 )
// 2 - 128x128 SSD1351 on the SPI bus  ( https://www.amazon.com/gp/product/B07DB5YFGW )
// #define SENSOR_DISPLAY 1

// Use this to start out in sensor display mode. 
// Useful if you're just testing a sensor and don't have any buttons hooked up yet.
// Only used if SENSOR_DISPLAY is non-zero
// #define SENSOR_DISPLAY_ON_STARTUP

// Turn this on to make the LED light up when buttons are pressed, and fade when they're released.
// #define BUTTON_LIGHTS

///////////////////////////////////////

#if SENSOR_DISPLAY == 1
  #define SENSOR_DISPLAY_GRAY4 1
  #define SENSOR_DISPLAY_I2C 1
  #include <Adafruit_SSD1327.h>
#elif SENSOR_DISPLAY == 2
  #define SENSOR_DISPLAY_COLOR565 1
  #define SENSOR_DISPLAY_SPI 1
  #include <Adafruit_SSD1351.h>
#endif


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

#if defined(PINS_QTPY)
  // Pin assignments on Seeeduino XIAO/Adafruit QT Py:
  // piezo speaker +
  #define PIN_PIEZO A3
  // Mouse button inputs
  #define PIN_BUTTON_LEFT A0 
  #define PIN_BUTTON_RIGHT A1 
  #define PIN_BUTTON_MIDDLE A2 

  // SPI Select pins for sensors
  // Sadly, these have different/incompatible definitions on the QT Py RP2040.
  #if defined(PINS_QTPY_RP2040)
    #define PIN_SENSOR_1_SELECT PIN_SERIAL2_RX  
    #define PIN_SENSOR_2_SELECT PIN_SERIAL2_TX  
  #else
    #define PIN_SENSOR_1_SELECT A7  
    #define PIN_SENSOR_2_SELECT A6  
  #endif

  // Testing: use software SPI
  // #define SENSOR_1_SOFTWARE_SPI     PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI
  // #define SENSOR_2_SOFTWARE_SPI     PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI

  // Defs for the Wire interface for the display
  // It will be plugged into the Stemma Qt plug
  #if defined(PINS_QTPY_RP2040)
    // On this board, the plug is Wire1
    #define DISPLAY_PIN_SDA PIN_WIRE1_SDA
    #define DISPLAY_PIN_SCL PIN_WIRE1_SCL
    #define DISPLAY_WIRE_DEVICE Wire1
  #else
    // On the original QT PY, the plug is just Wire
    #define DISPLAY_PIN_SDA PIN_WIRE_SDA
    #define DISPLAY_PIN_SCL PIN_WIRE_SCL
    #define DISPLAY_WIRE_DEVICE Wire
  #endif

#elif defined(PINS_CUSTOM_BOARD)
  // PIN_PIEZO is already defined in the variant's pins_arduino.h

   // Mouse button inputs
  #define PIN_BUTTON_LEFT PIN_B16 
  #define PIN_BUTTON_RIGHT PIN_B17 
  #define PIN_BUTTON_MIDDLE PIN_B18 

  // SPI Select pins for sensors

  // sensor 1
  // hardware SPI on SPI0.8 connector
  #define PIN_SENSOR_1_SELECT PIN_SPI0_SS08
  #define SENSOR_1_SPI_DEVICE SPI

  // sensor 2
#if 1
  // hardware SPI on SPI0.9 connector
  #define PIN_SENSOR_2_SELECT PIN_SPI0_SS09
  #define SENSOR_2_SPI_DEVICE SPI
#elif 0
  // hardware SPI on SPI1.13 connector
  #define PIN_SENSOR_2_SELECT PIN_SPI1_SS
  #define SENSOR_2_SPI_DEVICE SPI1
#else
  // software SPI on breakout connector
  #define PIN_SENSOR_2_SELECT PIN_BREAKOUT1
    // define these as sck, miso, mosi
  #define SENSOR_2_SOFTWARE_SPI   PIN_BREAKOUT4, PIN_BREAKOUT3, PIN_BREAKOUT2
#endif

  // Defs for the Wire interface for the display
  // It will be plugged into the Stemma Qt plug, a.k.a. Wire
  #define DISPLAY_PIN_SDA PIN_WIRE0_SDA
  #define DISPLAY_PIN_SCL PIN_WIRE0_SCL
  #define DISPLAY_WIRE_DEVICE Wire

#endif

// Button state polling/tracking
const char buttonNames[] = { MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE };
const int buttonPins[] = { PIN_BUTTON_LEFT, PIN_BUTTON_RIGHT,  PIN_BUTTON_MIDDLE };
const int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
char buttons;

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

// TODO: build a transform from the azimuth/elevation constants.

// The sensor transform
// Note that the sensor orientation I've been using all along is actually "sideways", for mechanical reasons
// i.e. the X axis of the sensor points up/down, and the Y axis points parallel to the desk.
// This transform takes that into account.
float st[3][4] = 
{
#if 1
  // This is the "hack" transform I've been using for the new sensor location 
  // (s1 at 180, s2 at  270 + 45, both at 30 degrees elevation)
  { -1,        0,       -sqrtf(2.0),  0   },  // X is s2.x, scaled up a bit, with s1.x subtracted to compensate for s2 being off-axis
  {  1,        0,        0,           0   },  // Y is s1.x
  {  0,       -0.5,      0,          -0.5 }   // Z is the average of the two sensors' y components.
#else
  // This was the transform for the original sensor location (s1 at 180 and s2 at 270, at zero elevation)
  {  0,        0,       -1,     0   },    // X is the inverse of the direct x reading of s2
  {  1,        0,        0,     0   },    // Y is the direct x reading of s1
  {  0,       -0.5,      0,    -0.5  }    // Z is the average of the two sensors' y components.
#endif
};

// sensor hardware abstraction
adns s1(PIN_SENSOR_1_SELECT, reported_cpi
#if defined(SENSOR_1_SPI_DEVICE)
  , SENSOR_1_SPI_DEVICE
#elif defined(SENSOR_1_SOFTWARE_SPI)
  , SENSOR_1_SOFTWARE_SPI
#endif
);

#if defined(PIN_SENSOR_2_SELECT)
  adns s2(PIN_SENSOR_2_SELECT, reported_cpi
  #if defined(SENSOR_2_SPI_DEVICE)
    , SENSOR_2_SPI_DEVICE
  #elif defined(SENSOR_2_SOFTWARE_SPI)
    , SENSOR_2_SOFTWARE_SPI
  #endif
  );
#else
  // make s2 a dummy sensor, for testing
  adns s2;
#endif

// scrolling
const int scroll_tick = 64;
float scroll_accum = 0;


#if SENSOR_DISPLAY == 1
  Adafruit_SSD1327 display(128, 128, &DISPLAY_WIRE_DEVICE, -1, 2000000UL);
  const int display_address = SSD1327_I2C_ADDRESS;
  const uint16_t text_color = SSD1327_WHITE;
  const uint16_t text_bg = SSD1327_BLACK;
#elif SENSOR_DISPLAY == 2
  #define DISPLAY_DC_PIN   4
  #define DISPLAY_CS_PIN   5
  #define DISPLAY_RST_PIN  -1
  Adafruit_SSD1351 display = Adafruit_SSD1351(128, 128, &SPI, DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN);
  const uint16_t text_color = 0xFFFF;
  const uint16_t text_bg = 0x0000;
#endif

#if defined(SENSOR_DISPLAY)
bool sensor_display_mode = false;
bool display_ready = false;
bool sensor_display_zoom_select = false;

void reset_display()
{
#if defined(SENSOR_DISPLAY_GRAY4)
  display.clearDisplay();
#elif defined(SENSOR_DISPLAY_COLOR565)
  display.fillScreen(text_bg);
#endif
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextColor(text_color, text_bg);
  display.setCursor(0,0);
  // Leave room for live updates
  display.print("");
  if (sensor_display_mode) {
    display.print(F("Press all 3 buttons\ntogether to return\nto trackball mode."));
#if defined(SENSOR_DISPLAY_GRAY4)
    display.setContrast(0x7f);
#endif
  } else {    
    display.print(F("Press all 3 buttons\ntogether to view \nsensor data."));
#if defined(SENSOR_DISPLAY_GRAY4)
    display.setContrast(0x1f);
#endif
  }
#if defined(SENSOR_DISPLAY_GRAY4)
  display.display();
#endif
}

void draw_sensor_pixels(int x, int y, uint8_t *pixels, int width, int height, size_t src_rowbytes, bool zoom, uint8_t min, uint8_t max)
{

#if defined(SENSOR_DISPLAY_GRAY4)
  uint8_t lut[256];
#elif defined(SENSOR_DISPLAY_COLOR565)
  uint16_t lut[256];
#endif
  // Scale the data to show maxumum contrast using the 16 gray levels available.
  // Scale the pixels so that the darkest gets value 0 and the brightest gets 15
  int range = max - min;
  uint8_t offset = min;

#if defined(SENSOR_DISPLAY_GRAY4)
  float scale = 1.0 / ((range > 0)?(range / 255.0):1.0);
  for(int i = min; i <=max; i++)
  {
    uint8_t color = (i - offset) * scale;
    lut[i] = (color >> 4) | (color & 0xf0);
  }

  // This blit converts 8 bit to 4 bit, and also optionally expands the data 2x in x and y.

  // Touch the pixels at the corners of this blit to update the dirty rect
  display.drawPixel(x, y, 0);
  display.drawPixel(x + ((zoom?2:1) * width) - 1, y + ((zoom?2:1) * width) - 1, 0);

  uint8_t *buffer = display.getBuffer();
  const int rowbytes = 64;
  // Assumptions:
  // X is even
  // the blit does not need to be clipped
  buffer += (rowbytes * y) + (x >> 1);

  for(int iy = 0; iy < height; iy++)
  {
    uint8_t *dst = buffer;
    if (zoom)
    {
      for(int ix = 0; ix < width; ix++)
      {
        uint16_t color = lut[pixels[ix]];
        // write two pixels at once, in two lines
        dst[0] = color;
        dst[rowbytes] = color;
        dst++;
      }
    } 
    else 
    {
      for(int ix = 0; ix < width; ix += 2)
      {
        uint16_t color = (lut[pixels[ix]] & 0xf0) | (lut[pixels[ix+1]] >> 4);
        // write two pixels at once, in two lines
        dst[0] = color;
        dst++;
      }

    }
    // move the buffer pointer down one or two rows
    buffer += rowbytes << (zoom?1:0);
    pixels += src_rowbytes;
  }
#elif defined(SENSOR_DISPLAY_COLOR565)
  float scale = 1.0 / ((range > 0)?(range / 255.0):1.0);
  for(int i = min; i <=max; i++){
    uint16_t color = (i - offset) * scale;
    lut[i] = display.color565(color, color, color);
  }

  // This blit converts 8 bit to 16 bit, and also optionally expands the data 2x in x and y.
  display.startWrite();
  display.setAddrWindow(x, y, width * (zoom?2:1), height * (zoom?2:1));

  for(int iy = 0; iy < height; iy++)
  {
    if (zoom)
    {    
      // write each line of pixels twice
      for(int j = 0; j < 2; j++)
      {
        for(int ix = 0; ix < width; ix++)
        {
          uint16_t color = lut[pixels[ix]];
          // write each pixel twice
          display.SPI_WRITE16(color);
          display.SPI_WRITE16(color);
        }
      }
    }
    else
    {
      // write each line of pixels once
      for(int ix = 0; ix < width; ix++)
      {
        uint16_t color = lut[pixels[ix]];
        display.SPI_WRITE16(color);
      }

    }
    pixels += src_rowbytes;
  }
  display.endWrite();
#endif

}

void display_sensors()
{
  // Largest known sensor image is 36 pixels square
  uint8_t pixels[36 * 36];
  // This reads the pixels off of both sensors, and draws them in the lower left and lower right corners of the display.
  // If there's room to do so without them overlapping, it magnifies both images by 2x.
  // If not, it only magnifies the one from s1.
  int width1 = s1.image_width();
  int height1 = s1.image_height();
  int width2 = s2.image_width();
  int height2 = s2.image_height();

  int zoom1 = 1;
  int zoom2 = 1;
  unsigned long read1 = micros();
  unsigned long render1 = read1;

  if (width1 != 0)
  {
    // Sensor 1 is present
    s1.read_image(pixels);
    render1 = micros();

    if (!sensor_display_zoom_select &&
        ((height1 * 2) + 2 < display.height()) &&
        ((width1 * 2) + 2 + width2 + 2 < display.width()))
    {
      zoom1 = 2;
    }

    int x1 = 0;
    int y1 = display.height() - ((height1 * zoom1) + 2);
    draw_sensor_pixels(x1 + 1, y1 + 1, pixels, width1, height1, width1, zoom1 == 2, s1.Minimum_Pixel, s1.Maximum_Pixel);
    display.drawRect(x1, y1, (width1 * zoom1) + 2, (height1 * zoom1) + 2, text_color);
  }
  unsigned long read2 = micros();
  unsigned long render2 = read2;

  if (width2 != 0)
  {
    s2.read_image(pixels);
    render2 = micros();

    zoom2 = (((height2 * 2) + 2 < display.height()) && ((width1 * zoom1) + 2 + (width2 * 2) + 2 < display.width()))?2:1;
    int x2 = display.width() - ((width2 * zoom2) + 2);
    int y2 = display.height() - ((height2 * zoom2) + 2);
    draw_sensor_pixels(x2 + 1, y2 + 1, pixels, width2, height2, width2, zoom2 == 2, s2.Minimum_Pixel, s2.Maximum_Pixel);
    display.drawRect(x2, y2, (width2 * zoom2) + 2, (height2 * zoom2) + 2, text_color);
  }

  unsigned long xfer = micros();
#if defined(SENSOR_DISPLAY_GRAY4)
  display.display();
#endif
  unsigned long end = micros();

  display.setCursor(0,24);
  display.printf("s1: 0x%02x - 0x%02x\ns2: 0x%02x - 0x%02x", 
    s1.Minimum_Pixel, s1.Maximum_Pixel,
    s2.Minimum_Pixel, s2.Maximum_Pixel);

  // Only log if at least one of the sensors is online
  if ((width1 != 0) || (width2 != 0))
  {
    debugLogger.print(F("read took "));
    debugLogger.print((render1 - read1) + (render2 - read2));
    debugLogger.print(F(", render took "));
    debugLogger.print((read2 - render1) + (xfer - render2));
    debugLogger.print(F(", xfer took "));
    debugLogger.println(end - xfer);
  }
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
#endif

#if defined(SENSOR_DISPLAY_I2C)
bool i2c_probe_bus()
{
  // Check to see whether it looks like there's anything connected to the SDA/SCL pins.
  // This is needed because calling Wire.begin() without anything conneted seems to break things.
  // Specifically, it returns false positives and sometimes hangs the device.

  // If i2c is connected, both pins should have pull-up resistors, which means they will read high.
  pinMode(DISPLAY_PIN_SDA, INPUT);
  pinMode(DISPLAY_PIN_SCL, INPUT);
  return (digitalRead(DISPLAY_PIN_SDA) == HIGH) && (digitalRead(DISPLAY_PIN_SCL) == HIGH);
}

bool i2c_probe_device(byte address)
{
  // Probe the specified address to see if a device seems to be present.
  DISPLAY_WIRE_DEVICE.beginTransmission(address);
  return (DISPLAY_WIRE_DEVICE.endTransmission() == 0);
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
#if defined(SERIAL_DEBUG)
  if (tud_cdc_connected())
  {
    result = Serial.write(c);
    yield();
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

#if defined(SERIAL_DEBUG)
  if (tud_cdc_connected())
  {
    result = Serial.write(buffer, size);
    yield();
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
#if defined(SERIAL_DEBUG)
  if (tud_cdc_connected())
  {
    result = true;
  }
#endif
  return result;
}

float ledRed = 0;
float ledGreen = 0;
float ledBlue = 0;

void setup() 
{
  // pinMode(LED_BUILTIN, OUTPUT);

  // Free up hardware serial pins for our use.
  Serial1.end();

  // TinyUSB Setup
  USBDevice.setProductDescriptor("MWTrackball");
  usb_hid.setStringDescriptor("MWTrackball");
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));

  usb_hid.begin();

  // wait until device mounted
  while( !USBDevice.mounted() ) delay(1);

#if defined(SERIAL_DEBUG)
  Serial.begin(115200);

#if 1
  // Wait for the serial port to be opened.
  // NOTE: this will block forever, which can be inconvenient if you're trying to use the device.
  while (!Serial) { delay(100); }
#else
  // Add a short delay so I can get the console open before things start happening.
  delay(2000);
#endif

  debugLogger.printf(("Opened serial port\n"));
#endif
  
  // disable the chip-select pins for both sensors (chip select is active-low)
  // adns::init() now does this internally
  // pinMode (PIN_SENSOR_1_SELECT, OUTPUT);
  // digitalWrite(PIN_SENSOR_1_SELECT, HIGH);
  // pinMode (PIN_SENSOR_2_SELECT, OUTPUT);
  // digitalWrite(PIN_SENSOR_2_SELECT, HIGH);

#if defined(DISPLAY_CS_PIN) 
  // also, disable chip-select for an SPI display if we're using one.
  pinMode (DISPLAY_CS_PIN, OUTPUT);
  digitalWrite(DISPLAY_CS_PIN, HIGH);
#endif

  // initialize the SPI device(s) for the sensors
  // adns::init() now calls begin() on the SPI device, so this is no longer necessary.
// #if defined(SENSOR_1_SPI_DEVICE)
//   debugLogger.println(F("Starting SENSOR_1_SPI_DEVICE"));
//   SENSOR_1_SPI_DEVICE.begin();
// #endif
// #if defined(SENSOR_2_SPI_DEVICE)
//   #if defined(SENSOR_1_SPI_DEVICE)
//     // If both are defined, and they're the same device, don't begin it twice.
//     if (&SENSOR_1_SPI_DEVICE != &SENSOR_2_SPI_DEVICE)
//   #endif
//   {
//     debugLogger.println(F("Starting SENSOR_2_SPI_DEVICE"));
//     SENSOR_2_SPI_DEVICE.begin();
//   }
// #endif

  debugLogger.printf("Initializing sensor 1:\n");
  bool s1_inited = s1.init();

  debugLogger.printf("Initializing sensor 2:\n");
  bool s2_inited = s2.init();

  // Since converting to Adafruit_SPIDevice, sometimes the first init attempt fails, 
  // for reasons I haven't yet figured out.
  // Retrying after initing the other sensor sometimes seems to work, which makes no sense.
  if (!s1_inited)
  {
    debugLogger.printf("Sensor 1 init failed, trying again:\n");
    s1.init();
  }

  if (!s2_inited)
  {
    debugLogger.printf("Sensor 2 init failed, trying again:\n");
    s2.init();
  }


  for(int i=0; i<buttonCount; i++)
  {
      pinMode(buttonPins[i], INPUT_PULLUP);
  }

#if defined(SENSOR_DISPLAY_I2C)
  if (!i2c_probe_bus())
  {
    debugLogger.printf("i2c bus appears disconnected\n");
  }
  else
  {
    debugLogger.printf("i2c bus seems sane\n");
    DISPLAY_WIRE_DEVICE.begin();
    if (!i2c_probe_device(display_address))
    {
      debugLogger.printf("Display not found\n");
    }
    else
    {
      debugLogger.printf("Display found\n");
      // Something responded at the correct address. Assume it's the display.
      if (!display.begin(display_address)) 
      {
        debugLogger.printf("Display init failed\n");
      }
      else
      {
        display_ready = true;
        debugLogger.printf("Display initialized\n");

        reset_display();
      }
    }
  }
#elif defined(SENSOR_DISPLAY_SPI) 
  // Communication with the SPI display is one-way, so just assume it's going to be there.
  display.begin();
  display_ready = true;
  reset_display();
#endif

#if defined(PIN_NEOPIXEL)

#if defined(NEOPIXEL_POWER)
  // Turn on power to the neopixel
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

  pixel.begin();  // initialize the pixel

  // and set its color
  pixel.setPixelColor(0, 0, 0, 0);
  pixel.show();
#endif

  debugLogger.printf("Initialization complete.\n");

#if defined(SENSOR_DISPLAY) && defined(SENSOR_DISPLAY_ON_STARTUP)
  set_sensor_display(true);
#endif
}

void click()
{
#if !defined(PIEZO_FREQUENCY)
  #define PIEZO_FREQUENCY 1500
#endif

#if !defined(PIEZO_DURATION)
  #define PIEZO_DURATION 5
#endif

#ifdef PIN_PIEZO
  pinMode(PIN_PIEZO, OUTPUT);
  // This is MUCH louder than just toggling the high/low once with digitalWrite().
  tone(PIN_PIEZO, PIEZO_FREQUENCY, PIEZO_DURATION);
#endif
}

void printBurst(adns &sensor)
{
    debugLogger.printf("Motion = 0x%02x, ", int(sensor.Motion));
    debugLogger.printf("Observation = 0x%02x, ", int(sensor.Observation));
    debugLogger.printf("SQUAL = 0x%02x, ", int(sensor.SQUAL));
    debugLogger.printf("Pixel_Sum = 0x%02x, ", int(sensor.Pixel_Sum));
    debugLogger.printf("min/max Pixel = 0x%02x/0x%02x, ", int(sensor.Minimum_Pixel), int(sensor.Maximum_Pixel));
    debugLogger.printf("Shutter = %d, ", sensor.Shutter);
    debugLogger.printf("Frame_period = %d, ", sensor.Frame_Period);
    debugLogger.printf("x/y = %d/%d", sensor.x, sensor.y);
    debugLogger.println("");
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

#if 0
    // Print all values from the burst.
    // Possibly useful for tweaking sensor parameters.
    debugLogger.printf("s1: ");
    printBurst(s1);
    debugLogger.printf("s2: ");
    printBurst(s2);
#endif

#if 0
    // Useful when debugging sensor scaling
    debugLogger.print(F("report_cpi = "));
    debugLogger.print(s1.report_cpi);
    debugLogger.print(F(", current_cpi = "));
    debugLogger.print(s1.current_cpi);
    debugLogger.print(F(", cpi_scale_factor = "));
    debugLogger.print(s1.cpi_scale_factor);
    debugLogger.print(F(", x = "));
    debugLogger.print(s1.x);
    debugLogger.print(F(", y = "));
    debugLogger.print(s1.y);
    debugLogger.print(F(", v = "));
    debugLogger.print(v1);
    debugLogger.println("");
#endif

    // Given the way my design mounts the sensors (with the wire attachment at the top), the 9800 is inverted relative to the others.
    if (s1.sensor_type() == adns::PID_adns9800)
    {
      v1 = -v1;
    }
    if (s2.sensor_type() == adns::PID_adns9800)
    {
      v2 = -v2;
    }

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
#if defined(BUTTON_LIGHTS)
        // For the first three buttons, set a color whenever the button is down
        switch(i)
        {
          case 0: ledRed = 1.0; break;
          case 1: ledGreen = 1.0; break;
          case 2: ledBlue = 1.0; break;
          default: break;
        }
#endif
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
      if (prevButtons == 0x07 && buttons != 0x07)
      {
        // All 3 buttons were pressed
        set_sensor_display(!sensor_display_mode);
      }
      else if (sensor_display_mode)
      {
          // If we're in sensor display mode, the main button toggles zoom focus
          if (prevButtons == 0x01 && buttons == 0x00)
          {
            sensor_display_zoom_select = !sensor_display_zoom_select;
            // Erase the sensor draw area
            const int max_sensor_draw_size = ((36 * 2) + 2);
            display.fillRect(
              0, 
              display.height() - max_sensor_draw_size,
              display.width(),
              max_sensor_draw_size,
              text_bg);
          }
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
    // This may be useful for debugging, but it brings us down to ~30hz with the i2c display.
#if 0
    display.fillRect(0, 0, 128, 16, 0);
    display.setCursor(0,0);
    display.println(delta);
    display.print(F("loop time "));
    display.print(loop_time / 1000);
    display.print(F("ms"));
#if defined(SENSOR_DISPLAY_GRAY4)
    display.display();
#endif
#endif
    ////////
    // Trying to diagnose an issue with kvm switching. This lets me see if the device resets on switch.
#if 0
    static unsigned long last_uptime_seconds = 0;
    unsigned long uptime_seconds = millis() / 1000;
    if (uptime_seconds != last_uptime_seconds)
    {
      last_uptime_seconds = uptime_seconds;
      // Default font is 6x8
      display.setCursor(0,0);
      display.printf("%02d:%02d:%02d", uptime_seconds / 3600, (uptime_seconds / 60) % 60, (uptime_seconds) % 60);
#if defined(SENSOR_DISPLAY_GRAY4)
      display.display();
#endif
    }
#endif

  }
#endif

#if defined(BUTTON_LIGHTS)
  // Update the lighting
  pixel.setPixelColor(0, ledRed * 0xFF, ledGreen * 0xFF, ledBlue * 0xFF);
  pixel.show();

  // Fade in approximately half a second
  static float fadeVal = 1.0 / (report_Hz / 2);
  
  ledRed -= fadeVal;
  if (ledRed < 0)
    ledRed = 0; 
  ledGreen -= fadeVal;
  if (ledGreen < 0)
    ledGreen = 0; 
  ledBlue -= fadeVal;
  if (ledBlue < 0)
    ledBlue = 0; 
#endif

  // Delay to keep the loop time right around report_microseconds
  loop_time = micros() - loop_start_time;
  if (loop_time < report_microseconds)
  {
    delayMicroseconds(report_microseconds - loop_time);
  }
}


