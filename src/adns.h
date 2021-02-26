
class adns
{
public:
    adns(int ncs, int report_cpi);
    // Sets up the chip select for this instance and initializes the chip (upload firmware, etc).
    void init ();
    // Polls the sensor and returns motion vector scaled to report_cpi
    Vector motion();

private:
    // The arduino pin number this chip's chip select is tied to.
    int ncs;

    void upload_firmware();
    void com_begin();
    void com_end();

    byte read_reg(byte reg_addr);
    void write_reg(byte reg_addr, byte data);

    void read_motion_burst();
    void set_cpi(int cpi);
    void set_snap_angle(byte enable);    

    void dispRegisters(void);

    int x;
    int y;
    int current_cpi;
    int report_cpi;
    double cpi_scale_factor;

};
