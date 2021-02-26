
class adns
{
public:
    adns(int ncs, int report_cpi);
    // Sets up the chip select for this instance and initializes the chip (upload firmware, etc).
    void init ();
    // Polls the sensor and returns motion vector scaled to report_cpi
    Vector motion();

    // This puts the sensor into image capture mode. 
    // While in this mode, it will not report motion.
    void begin_image_capture();
    // This reads pixels from the sensor when in capture mode.
    // It will store 900 bytes (one byte per pixel) to the specified address.
    void read_image(uint8_t *pixels);
    // This ends image capture mode and puts the sensor back into motion tracking mode.
    void end_image_capture();

    // All of these are filled in each time we do a burst motion read.
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
    void upload_firmware();
    void enable_laser();
    void com_begin();
    void com_end();

    byte read_reg(byte reg_addr);
    void write_reg(byte reg_addr, byte data);

    void read_motion_burst();
    void set_cpi(int cpi);
    void set_snap_angle(byte enable);    

    void dispRegisters(void);

    int current_cpi;
    int report_cpi;
    double cpi_scale_factor;
    bool capture_mode;

};
