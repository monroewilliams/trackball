// These defines determine which sensor firmware gets brought in by this code.
// Removing the define for sensor types you're not using will save you around 4KB of flash for each one.
// It's fine to leave both of them defined, the code will probe the sensor type at runtime and use 
// the correct firmware.
#define ADNS_SUPPORT_ADNS9800 1
#define ADNS_SUPPORT_PMW3360DM 1
#define ADNS_SUPPORT_PMW3389DM 1
class adns
{
public:
    adns(int ncs, int report_cpi);
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
        PID_pmw3389dm = 0x47,   // This is the product ID from the datasheet, but the code doesn't really support this chip yet.
    };

    void set_cpi(int cpi);
    int updateCPI(int increment);

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

private:
    // The arduino pin number this chip's chip select is tied to.
    int ncs;

    void reset();
    
    bool upload_firmware();
    void enable_laser();
    // default to 2MHz bus speed for most transactions
    void com_begin(uint32_t clock = 2000000);
    void com_end();

    byte read_reg(byte reg_addr);
    void write_reg(byte reg_addr, byte data);

    void read_motion_burst();
    
    void set_snap_angle(byte enable);    

    void dispRegisters(void);

    int product_id;
    int current_cpi;
    int report_cpi;
    double cpi_scale_factor;

    int chip_state;

};
