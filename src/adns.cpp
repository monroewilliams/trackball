#include <Arduino.h>
#include <SPI.h>

#include "trackball.h"
#include "Vector.h"
#include "adns.h"

#ifdef ADNS_SUPPORT_ADNS9800
  #include "ADNS9800_firmware.h"
#endif
#ifdef ADNS_SUPPORT_PMW3360DM
  #include "PMW3360DM_firmware.h"
#endif
#ifdef ADNS_SUPPORT_PMW3389DM
  #include "PMW3389DM_firmware.h"
#endif

// Registers
enum
{
  REG_Product_ID                           = 0x00,
  REG_Revision_ID                          = 0x01,
  REG_Motion                               = 0x02,
  REG_Delta_X_L                            = 0x03,
  REG_Delta_X_H                            = 0x04,
  REG_Delta_Y_L                            = 0x05,
  REG_Delta_Y_H                            = 0x06,
  REG_SQUAL                                = 0x07,
  REG_Pixel_Sum                            = 0x08,
  REG_Maximum_Pixel                        = 0x09,
  REG_Minimum_Pixel                        = 0x0a,
  REG_Shutter_Lower                        = 0x0b,
  REG_Shutter_Upper                        = 0x0c,
  REG_Control                              = 0x0d,    // PMW3360DM only
  REG_Frame_Period_Lower                   = 0x0d,    // ADNS-9800 only
  REG_Frame_Period_Upper                   = 0x0e,    // ADNS-9800 only
  REG_Resolution_H                         = 0x0e,    // PMW3389-only name
  REG_Resolution_L                         = 0x0f,    // PMW3389-only name
  REG_Configuration_I                      = 0x0f,
  REG_Configuration_II                     = 0x10,
  REG_Angle_Tune                           = 0x11,    // PMW3360DM only
  REG_Frame_Capture                        = 0x12,
  REG_SROM_Enable                          = 0x13,
  REG_Run_Downshift                        = 0x14,
  REG_Rest1_Rate                           = 0x15,
  REG_Rest1_Downshift                      = 0x16,
  REG_Rest2_Rate                           = 0x17,
  REG_Rest2_Downshift                      = 0x18,
  REG_Rest3_Rate                           = 0x19,
  REG_Frame_Period_Max_Bound_Lower         = 0x1a,
  REG_Frame_Period_Max_Bound_Upper         = 0x1b,
  REG_Frame_Period_Min_Bound_Lower         = 0x1c,
  REG_Frame_Period_Min_Bound_Upper         = 0x1d,
  REG_Shutter_Max_Bound_Lower              = 0x1e,
  REG_Shutter_Max_Bound_Upper              = 0x1f,
  REG_LASER_CTRL0                          = 0x20,    // ADNS-9800 only
  REG_Observation                          = 0x24,
  REG_Data_Out_Lower                       = 0x25,
  REG_Data_Out_Upper                       = 0x26,
  REG_SROM_ID                              = 0x2a,
  REG_Lift_Detection_Thr                   = 0x2e,
  REG_Configuration_V                      = 0x2f,
  REG_Configuration_IV                     = 0x39,
  REG_Power_Up_Reset                       = 0x3a,
  REG_Shutdown                             = 0x3b,
  REG_Inverse_Product_ID                   = 0x3f,
  REG_Snap_Angle                           = 0x42,
  REG_Motion_Burst                         = 0x50,
  REG_SROM_Load_Burst                      = 0x62,
  REG_Pixel_Burst                          = 0x64,
};

// Delay times from the datasheet, in microseconds
enum
{
    mcs_tSWW = 120,          // SPI time between write commands
    mcs_tSWR = 120,          // SPI time between write and read commands
    mcs_tSRW = 20,           // SPI time between read and subsequent commands
    mcs_tSRR = 20,           // SPI time between read and subsequent commands
    mcs_tSRAD = 100,         // SPI read address-data delay
    mcs_tSCLK_NCS_read = 1,  // SCLK to NCS inactive (for read operation) (actually 120 nanoseconds)
    mcs_tSCLK_NCS_write = 20, // SCLK to NCS inactive (for read operation)
    mcs_tBEXIT = 1,           // NCS inactive after motion burst (actually 500 nanoseconds)
    mcs_tSRAD_MOTBR = 35,      // From rising SCLK for last bit of the address byte, to falling SCLK for first bit of data being read. Applicable for Burst Mode Motion Read only.
};

enum
{
  chip_state_uninitialized = 0,
  chip_state_motion = 1,
  chip_state_image_capture = 2
};

static inline int bytes2int(byte h, byte l)
{
  int result = (int8_t)h;
  result *= 256;
  result += l;
  return result;
}

#define CLAMP(val, min, max) (val > max)?max:((val < min)?min:val)

static const uint32_t bitrate = 200000;

adns::adns(int8_t ncs, int report_cpi)
    : report_cpi(report_cpi)
#if !defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
    , ncs(ncs)
    , settings(bitrate, MSBFIRST, SPI_MODE3)
#endif
{  
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  spi_dev = new Adafruit_SPIDevice(ncs, bitrate, SPI_BITORDER_MSBFIRST, SPI_MODE3);
#else
  spi_dev = &SPI;
#endif
  common_construct();
}

adns::adns(int8_t ncs, int report_cpi, SPIClass &spi)
    : report_cpi(report_cpi)
#if !defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
    , ncs(ncs)
    , settings(bitrate, MSBFIRST, SPI_MODE3)
#endif
{
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  spi_dev = new Adafruit_SPIDevice(ncs, bitrate, SPI_BITORDER_MSBFIRST, SPI_MODE3, &spi);
#else
  spi_dev = &spi;
#endif
  common_construct();
}

#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
adns::adns(int8_t ncs, int report_cpi, int8_t sck, int8_t miso, int8_t mosi)
    : report_cpi(report_cpi)
{
  // Software SPI seems to want a slower clock to function correctly?
  spi_dev = new Adafruit_SPIDevice(ncs, sck, miso, mosi, 100000, SPI_BITORDER_MSBFIRST, SPI_MODE3);
  common_construct();
}
#endif

adns::adns(void)
    : report_cpi(0)
#if !defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
    , ncs(-1)
    , settings(bitrate, MSBFIRST, SPI_MODE3)
#endif
{
  spi_dev = NULL;
  common_construct();
}

void adns::common_construct()
{
  chip_state = chip_state_uninitialized;
  product_id = PID_unknown;
}

adns::~adns()
{
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  // In this case, spi_dev is an Adafruit_SPIDevice we allocated, so we need to clean it up.
  if (spi_dev)
    delete spi_dev;
#endif
}

bool adns::init ()
{
  if (!spi_dev)
  {
    debugLogger.printf("adns::init: dummy device\n");
    return false;
  }

#if !defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  pinMode(ncs, OUTPUT);
  digitalWrite(ncs, HIGH);
#endif

  spi_dev->begin();

  chip_state = chip_state_uninitialized;

  reset();

  // Read the product ID, so we know which firmware to use.
  product_id = read_reg(REG_Product_ID);
  debugLogger.printf("Read product id: 0x%02x\n", int(product_id));
  delay(1);

  // upload the firmware
  if (!upload_firmware())
  {
    return false;
  }

  enable_laser();

  debugLogger.printf("Chip initialized\n");

  // By default, run the sensor at its maximum CPI value.  
  switch(product_id)
  {
    case PID_adns9800:
      set_cpi(8200);
    break;
    case PID_pmw3360dm:
      set_cpi(12000);
    break;
    case PID_pmw3389dm:
      set_cpi(16000);
    break;
    default:
    break;
  }


  delay(1);
  
  dispRegisters();
  
  delay(100);

  chip_state = chip_state_motion;

  return true;
}

void adns::reset()
{
  if (!spi_dev)
  {
      return;
  }

  com_end(); // ensure that the SPI port is reset
  com_begin(); // ensure that the SPI port is reset
  com_end(); // ensure that the SPI port is reset
  write_reg(REG_Power_Up_Reset, 0x5a); // force reset
  delay(50); // wait for it to reboot

  // read registers 0x02 to 0x06 (and discard the data)
  read_reg(REG_Motion);
  read_reg(REG_Delta_X_L);
  read_reg(REG_Delta_X_H);
  read_reg(REG_Delta_Y_L);
  read_reg(REG_Delta_Y_H);

  delay(50);
}

void adns::enable_laser()
{
  // This is only necessary on the 9800
  if (product_id == PID_adns9800)
  {
    // enable laser(bit 0 = 0b), in normal mode (bits 3,2,1 = 000b)
    // reading the actual value of the register is important because the real
    // default value is different from what is said in the datasheet, and if you
    // change the reserved bytes (like by writing 0x00...) it would not work.
    byte laser_ctrl0 = read_reg(REG_LASER_CTRL0);
    write_reg(REG_LASER_CTRL0, laser_ctrl0 & 0xf0 );  
  }
}

void adns::com_begin(bool fast)
{
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  spi_dev->beginTransactionWithAssertingCS();
#else
  if (fast)
  {
      spi_dev->beginTransaction(SPISettings(3200000, MSBFIRST, SPI_MODE3));
  }
  else
  {
    spi_dev->beginTransaction(settings);
  }
  digitalWrite(ncs, LOW);
#endif
}

void adns::com_end()
{
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
  spi_dev->endTransactionWithDeassertingCS();
#else
  digitalWrite(ncs, HIGH);
  spi_dev->endTransaction();
#endif
}

byte adns::read_reg(byte reg_addr)
{
  com_begin();
  
  // send adress of the register, with MSBit = 0 to indicate it's a read
  spi_dev->transfer(reg_addr & 0x7f );
  delayMicroseconds(mcs_tSRAD);
  // read data
  byte data = spi_dev->transfer(0);
  
  delayMicroseconds(mcs_tSCLK_NCS_read);
  com_end();
  delayMicroseconds(mcs_tSRW - mcs_tSCLK_NCS_read);

  // This is too spammy during normal use, but can be useful when debugging sensor issues.
  if (0)
  {
      debugLogger.printf("read register 0x%02x, value = 0x%02x\n", int(reg_addr), int(data));
  }
  
  return data;
}

void adns::write_reg(byte reg_addr, byte data)
{
  // This is too spammy during normal use, but can be useful when debugging sensor issues.
  if (0)
  {
      debugLogger.printf("write register 0x%02x, value = 0x%02x\n", int(reg_addr), int(data));
  }

  com_begin();
  
  //send adress of the register, with MSBit = 1 to indicate it's a write
  spi_dev->transfer(reg_addr | 0x80 );
  //sent data
  spi_dev->transfer(data);
  
  delayMicroseconds(mcs_tSCLK_NCS_write); // tSCLK-NCS for write operation
  com_end();
  delayMicroseconds(mcs_tSWW - mcs_tSCLK_NCS_write); // Could be shortened, but is looks like a safe lower bound 
}

bool adns::upload_firmware()
{  
  // send the firmware to the chip, cf p.18 of the datasheet

  unsigned short firmware_length;
  const uint8_t *firmware_data;
  switch(product_id)
  {
#ifdef ADNS_SUPPORT_ADNS9800
    case PID_adns9800:
      debugLogger.printf("Uploading ADNS-9800 firmware\n");
      firmware_length = firmware_length_adns9800;
      firmware_data = firmware_data_adns9800;

      // set the configuration_IV register in 3k firmware mode
      write_reg(REG_Configuration_IV, 0x02); // bit 1 = 1 for 3k mode, other bits are reserved 
    break;
#endif
#ifdef ADNS_SUPPORT_PMW3360DM
    case PID_pmw3360dm:
      debugLogger.printf("Uploading PMW3360DM firmware\n");
      firmware_length = firmware_length_pmw3360dm;
      firmware_data = firmware_data_pmw3360dm;

      // Write 0 to Rest_En bit of Config2 register to disable Rest mode.
      write_reg(REG_Configuration_II, 0x20);
    break;
#endif
#ifdef ADNS_SUPPORT_PMW3389DM
    case PID_pmw3389dm:
      debugLogger.printf("Uploading PMW3389DM firmware\n");
      firmware_length = firmware_length_pmw3389dm;
      firmware_data = firmware_data_pmw3389dm;

      // Write 0 to Rest_En bit of Config2 register to disable Rest mode.
      write_reg(REG_Configuration_II, 0x20);
    break;
#endif

    default:
      debugLogger.printf("*** No firmware available for this chip! ***\n");
      return false;
    break;
  }
  
  // write 0x1d in SROM_enable reg for initializing
  write_reg(REG_SROM_Enable, 0x1d); 
  
  // wait for more than one frame period
  delay(10); // assume that the frame rate is as low as 100fps... even if it should never be that low
  
  // write 0x18 to SROM_enable to start SROM download
  write_reg(REG_SROM_Enable, 0x18); 
  
  // write the SROM file (=firmware data) 
  com_begin();
  spi_dev->transfer(REG_SROM_Load_Burst | 0x80); // write burst destination adress
  delayMicroseconds(15);
  
  // send all bytes of the firmware
  unsigned char c;
  for(unsigned int i = 0; i < firmware_length; i++)
  { 
    c = (unsigned char)pgm_read_byte(firmware_data + i);
    spi_dev->transfer(c);
    delayMicroseconds(15);
  }

  com_end();

  delay(10);

#if 1
  // SROM CRC test
  write_reg(REG_SROM_Enable, 0x15); 
  delay(10);
  int upper = read_reg(REG_Data_Out_Upper);
  int lower = read_reg(REG_Data_Out_Lower);
  // The datasheet doesn't specify what the expected value for a successful CRC test is.
  // A successful test on the 3360 seems to return the value 0xbeef.
  debugLogger.printf("SROM CRC test result: 0x%02x%02x\n", upper, lower);
#endif

  switch(product_id)
  {
    case PID_pmw3360dm:
      // Read the SROM_ID register to verify the ID before any other register reads or writes.
      read_reg(REG_SROM_ID);

      // Write 0x00 to Config2 register for wired mouse or 0x20 for wireless mouse design.
      write_reg(REG_Configuration_II, 0x00);

      // set initial CPI resolution
      // We do this later, using set_cpi().
      // write_reg(REG_Configuration_I, 0x15);
    break;

    case PID_pmw3389dm:
      // Read the SROM_ID register to verify the ID before any other register reads or writes.
      read_reg(REG_SROM_ID);

      // Write 0x00 to Config2 register for wired mouse or 0x20 for wireless mouse design.
      write_reg(REG_Configuration_II, 0x00);

      // set initial CPI resolution
      // We do this later, using set_cpi().
      // write_reg(REG_Configuration_I, 0x15);
    break;
  }

  return true;
}

void adns::dispRegisters(void)
{
  if (debugLogger.enabled())
  {
    typedef struct
    {
      int id;
      const char *name;
    } regInfo;
    #define REG(x) { REG_##x, #x }
    regInfo oreg[] = 
    {
      REG(Product_ID), 
      REG(Inverse_Product_ID),
      REG(SROM_ID),
      REG(Motion), 
      REG(Configuration_I), 
      REG(Lift_Detection_Thr)
    };
    #undef REG

    byte regres;

    for(unsigned int rctr=0; rctr<(sizeof(oreg)/sizeof(oreg[0])); rctr++)
    {
      regres = read_reg(oreg[rctr].id);
      debugLogger.printf("%s (0x%02x) = 0x%02x / 0b",
        oreg[rctr].name,
        int(oreg[rctr].id),
        int(regres)
      );
      // printf doesn't have a binary conversion, so just use the builtin.
      debugLogger.println(regres, BIN);  
      delay(1);
    }
  }
}

Vector adns::motion()
{
  if (chip_state != chip_state_motion)
  {
    // In capture mode, we have no motion tracking.
    return Vector(0, 0);
  }
  read_motion_burst();
  return Vector(x * cpi_scale_factor, y * cpi_scale_factor);
}

void adns::read_motion_burst()
{
  byte burst[14];
  
  com_begin();

  // Read the burst register to start the transfer
  spi_dev->transfer(REG_Motion_Burst & 0x7F );
  delayMicroseconds(mcs_tSRAD_MOTBR);
  for (int i = 0; i < 14; i++)
  {
      burst[i] = spi_dev->transfer(0x00);
  }

  com_end();
  delayMicroseconds(mcs_tBEXIT);
  
  // Clear residual motion by writing the Motion register
  write_reg(REG_Motion, 0x00);
  
  // Extract burst values
  Motion = burst[0];
  Observation = burst[1];
  x = bytes2int(burst[3], burst[2]);  
  y = bytes2int(burst[5], burst[4]);  
  SQUAL = burst[6];
  Pixel_Sum = burst[7];
  Maximum_Pixel = burst[8];
  Minimum_Pixel = burst[9];
  Shutter = bytes2int(burst[10], burst[11]);
  Frame_Period = bytes2int(burst[12], burst[13]);

  if (debugLogger.enabled())
  {
#if 0
    debugLogger.print(F("burst motion data:"));
    for (int i = 0; i < 14; i++)
    {
      debugLogger.print(burst[i], HEX);
      debugLogger.print(F(" "));
    }
    debugLogger.print(F(", x = "));
    debugLogger.print(x);
    debugLogger.print(F(", y = "));
    debugLogger.println(y);
#endif
  }
  
}

void adns::set_cpi(int cpi)
{
    // Save the current CPI and calculate the divisor to use when reporting motion.
    current_cpi = cpi;
    cpi_scale_factor = report_cpi;
    cpi_scale_factor /= cpi;

    debugLogger.printf("set_cpi(): cpi = %d, report_cpi = %d, cpi_scale_factor = %lf",
      cpi,
      report_cpi,
      cpi_scale_factor);

    // Setting the sensor cpi differs by sensor model.
    switch(product_id)
    {
      case PID_adns9800:
        // The ADNS-9800 datasheet says that the value of REG_Configuration_I specifies the resolution in units of 50 cpi,
        //     with a minimum of 0x01 (50 cpi) and a maximum of 0xA4 (8200cpi).
        cpi /= 50;
        cpi = CLAMP(cpi, 1, 0xA4);
        write_reg(REG_Configuration_I, cpi);
        debugLogger.printf(", REG_Configuration_I = 0x%02x", cpi);
      break;
      case PID_pmw3360dm:
        // The pmw3360DM datasheet I have defines register 0x0F as "Config1", but does not specifically define allowable bit values.
        //     It does say that the chip has "selectable resolutions up to 12000cpi with 100cpi step size", and that the default value
        //     for the register is 0x31.
        cpi /= 100;
        cpi = CLAMP(cpi, 1, (12000 / 100));
        write_reg(REG_Configuration_I, cpi);
        debugLogger.printf(", REG_Configuration_I = 0x%02x", cpi);
      break;
      case PID_pmw3389dm:
        // The pmw3389DM datasheet I have defines register 0x0F as "Resolution_L" and register 0x0E as "Resolution_H", and also does not
        //     define allowable values. It says that the chip has "selectable resolutions up to 16000CPI with 50CPI step size", and shows
        //     default values of 0x42 for Resolution_L and 0x0 for Resolution_H.
        //     16000 / 50 comes out to 320 (> 255), so it makes sense that they would need an additional 8 bit register for the full range.
        cpi /= 50;
        cpi = CLAMP(cpi, 1, (16000 / 50));
        write_reg(REG_Resolution_L, cpi & 0x00FF);
        write_reg(REG_Resolution_H, cpi >> 8);
        debugLogger.printf(", REG_Resolution_L = 0x%02x, REG_Resolution_H = 0x%02x",
          cpi & 0x00FF,
          cpi >> 8);
      break;
      default:
      break;
    }
    debugLogger.printf("\n");
}

void adns::set_snap_angle(byte enable)
{
    write_reg(REG_Snap_Angle, enable?0x80:0x00);
}

int adns::image_width()
{
  int result = 0;
  switch(product_id)
  {
    case PID_adns9800:
      result = 30;
    break;
    case PID_pmw3360dm:
      result = 36;
    break;
    case PID_pmw3389dm:
      result = 36;
    break;
    default:
    break;
  }
  return result;  
}
int adns::image_height()
{
  // All known sensors return square images.
  return image_width();
}

size_t adns::image_data_size()
{
  return image_width() * image_height();
}


///// Capturing images from the sensor, per the datasheet:
// 1. Reset the chip by writing 0x5a to Power_Up_Reset register (address 0x3a).
// 2. Enable laser by setting Forced_Disable bit (Bit-0) of LASER_CTRL) register to 0.
// {
//     3. Write 0x93 to Frame_Capture register.
//     4. Write 0xc5 to Frame_Capture register.
//     5. Wait for two frames.
//     6. Check for first pixel by reading bit zero of Motion register. If = 1, first pixel is available.
//     7. Continue read from Pixel_Burst register until all 900 pixels are transferred.
//     8. Continue step 3-7 to capture another frame.
// }

void adns::begin_image_capture()
{
  if (!spi_dev)
  {
      return;
  }

  reset();
  enable_laser();
  chip_state = chip_state_image_capture;
}

void adns::read_image(uint8_t *pixels)
{
  if (chip_state != chip_state_image_capture)
  {
    // This only works after begin_image_capture has been called.
    return;
  }

  size_t datasize = image_data_size();

  write_reg(REG_Frame_Capture, 0x93 );
  write_reg(REG_Frame_Capture, 0xc5 );

  // I'm not sure how long "two frames" is. Trying this.
  delayMicroseconds(1000); 

  // The bit in the adns9800 datasheet about reading the Motion register doesn't seem to do anything useful.
  // I suspect it may be a mis-print. This code works on the hardware I have (both adns9800 and pmw3360)

  // Bump up the SPI speed a bit for this transaction, since there's a fair bit of data to move.
  com_begin(true);

  // Since we're touching all the pixels anyway, calculate the min and max values as we read them.
  uint8_t min = 255;
  uint8_t max = 0;

  // Reading a value from this register starts burst mode.
  // The value of this first read seems to be garbage, so just discard it.
  spi_dev->transfer(REG_Pixel_Burst & 0x7f);
  delayMicroseconds(mcs_tSRAD);

    for(size_t i = 0; i < datasize; i++)
    {
        uint8_t cur = spi_dev->transfer(REG_Pixel_Burst & 0x7F );
        pixels[i] = cur;
        if (cur < min) min = cur;
        if (cur > max) max = cur;
        delayMicroseconds(5);
    }

  delayMicroseconds(mcs_tBEXIT);
  com_end();

  Minimum_Pixel = min;
  Maximum_Pixel = max;
}

void adns::end_image_capture()
{
  if (!spi_dev)
  {
      return;
  }
  // Just reinitialize the sensor and restore the previously set CPI.
  int previous_cpi = current_cpi;
  init();
  set_cpi(previous_cpi);
}


