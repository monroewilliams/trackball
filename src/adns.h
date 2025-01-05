// These defines determine which sensor firmware gets brought in by this code.
// Removing the define for sensor types you're not using will save you around 4KB of flash for each one.
// It's fine to leave both of them defined, the code will probe the sensor type at runtime and use 
// the correct firmware.
#define ADNS_SUPPORT_ADNS9800 1
#define ADNS_SUPPORT_PMW3360DM 1
#define ADNS_SUPPORT_PMW3389DM 1

// Uncomment this (or define this symbol using build_flags in platformio.ini) to use the Adafruit_SPIDevice abstraction from the Adafrit BusIO library.
// This _should_ allow the use of software SPI if needed, although I'm having difficulty getting it to work properly on rp2040.
// #define ADNS_USE_SPIDEVICE_ABSTRACTION

#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
#include <Adafruit_SPIDevice.h>
#endif

class adns
{
public:
    // Set up using the default SPI device
    adns(int8_t ncs, int report_cpi);
    // Set up using a specific SPI device
    adns(int8_t ncs, int report_cpi, SPIClass &spi);

#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
    // Set up using software SPI
    adns(int8_t ncs, int report_cpi, int8_t sck, int8_t miso, int8_t mosi);
#endif

    // Create a dummy instance which does nothing (useful for testing, perhaps)
    adns();

    ~adns();

    // Sets up the chip select for this instance and initializes the chip (upload firmware, etc).
    // Returns true if successful.
    bool init ();
    // Polls the sensor and returns motion vector scaled to report_cpi
    Vector motion();

    // These give the size of the sensor image that read_image will return.
    int image_width();
    int image_height();
    size_t image_data_size();

    // This puts the sensor into image capture mode. 
    // While in this mode the hardware will not track motion, and calling motion() will do nothing and return (0, 0).
    void begin_image_capture();

    // This reads pixels from the sensor when in capture mode.
    // It will store image_data_size() bytes (one byte per pixel) to the specified address.
    void read_image(uint8_t *pixels);

    // This ends image capture mode and puts the sensor back into motion tracking mode.
    void end_image_capture();

    // Which type of sensor was detected. Expected values are from the following enum.
    int sensor_type() { return product_id; };
    enum
    {
        // Types of sensor recognized by this code.
        PID_unknown = 0x00,
        PID_adns9800 = 0x33,
        PID_pmw3360dm = 0x42,
        PID_pmw3389dm = 0x47,   // pmw3389 support contributed by https://github.com/JPJrunesJPJ
    };

    // These are all the fields in the burst motion response.
    // All of them are filled in each time motion() is called.
    // Some of them may be useful.
    byte Motion;
    byte Observation;
    int x;
    int y;
    byte SQUAL;
    byte Pixel_Sum;
    byte Maximum_Pixel;
    byte Minimum_Pixel;
    int Shutter;
    int Frame_Period;

    // These are public so that code that wants to do tricky things with dynamic CPI adjustment can use them. 
    // Make sure you understand how the variables are used internally before you modify them.
   
    // This method sets the cpi value which the sensor is asked to report at. 
    // By default, the initializer will set this to the maximum cpi that the detected model of sensor is capable of.
    // Calling this changes the hardware registers on the sensor to report at the specified cpi, 
    // and also uses the current value of report_cpi to recalculate cpi_scale_factor.
    void set_cpi(int cpi);
    
    // This is the cpi that the caller wants to be reported back when calling motion().
    // The values read from the sensor will be scaled to this cpi.
    int report_cpi;

    // This is the cpi value that the sensor is currently running at.
    int current_cpi;
    
    // This is the scaling factor used to translate from current_cpi to report_cpi when motion() is called.
    double cpi_scale_factor;

private:
    // SPI device abstraction
#if defined(ADNS_USE_SPIDEVICE_ABSTRACTION)
    Adafruit_SPIDevice *spi_dev;
#else
    int ncs;
    SPISettings settings;
    SPIClass *spi_dev;
#endif

    void common_construct();
    void reset();
    
    bool upload_firmware();
    void enable_laser();
    void com_begin(bool fast = false);
    void com_end();

    byte read_reg(byte reg_addr);
    void write_reg(byte reg_addr, byte data);

    void read_motion_burst();
    
    void set_snap_angle(byte enable);    

    void dispRegisters(void);

    int product_id;

    int chip_state;

};
