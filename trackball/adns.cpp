#include <SPI.h>
#include <avr/pgmspace.h>

#include "adns.h"

extern "C"
{
    extern byte serial_debug;
}


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
};

void adns::init (int chip_select)
{
  this->ncs = chip_select;

  pinMode (this->ncs, OUTPUT);

  this->com_end(); // ensure that the serial port is reset
  this->com_begin(); // ensure that the serial port is reset
  this->com_end(); // ensure that the serial port is reset
  this->write_reg(REG_Power_Up_Reset, 0x5a); // force reset
  delay(50); // wait for it to reboot
  // read registers 0x02 to 0x06 (and discard the data)
  this->read_reg(REG_Motion);
  this->read_reg(REG_Delta_X_L);
  this->read_reg(REG_Delta_X_H);
  this->read_reg(REG_Delta_Y_L);
  this->read_reg(REG_Delta_Y_H);
  // upload the firmware
  this->upload_firmware();
  delay(10);
  //enable laser(bit 0 = 0b), in normal mode (bits 3,2,1 = 000b)
  // reading the actual value of the register is important because the real
  // default value is different from what is said in the datasheet, and if you
  // change the reserved bytes (like by writing 0x00...) it would not work.
  byte laser_ctrl0 = this->read_reg(REG_LASER_CTRL0);
  this->write_reg(REG_LASER_CTRL0, laser_ctrl0 & 0xf0 );
  
  if (serial_debug)
  {
    Serial.println("Chip initialized");
  }
  
  delay(1);
  
  if (serial_debug)
  {
    this->dispRegisters();
  }
  
  delay(100);
}

void adns::com_begin()
{
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
  digitalWrite(this->ncs, LOW);
}

void adns::com_end()
{
  digitalWrite(this->ncs, HIGH);
  SPI.endTransaction();
}

byte adns::read_reg(byte reg_addr)
{
  this->com_begin();
  
  // send adress of the register, with MSBit = 0 to indicate it's a read
  SPI.transfer(reg_addr & 0x7f );
  delayMicroseconds(mcs_tSRAD);
  // read data
  byte data = SPI.transfer(0);
  
  delayMicroseconds(mcs_tSCLK_NCS_read);
  this->com_end();
  delayMicroseconds(mcs_tSRW - mcs_tSCLK_NCS_read);
  
  if (0)
  {
      Serial.print("read register ");
      Serial.print(reg_addr,HEX);
      Serial.print(", value = ");
      Serial.println(data,HEX);
  }
  
  return data;
}

void adns::write_reg(byte reg_addr, byte data)
{
  this->com_begin();
  
  //send adress of the register, with MSBit = 1 to indicate it's a write
  SPI.transfer(reg_addr | 0x80 );
  //sent data
  SPI.transfer(data);
  
  delayMicroseconds(mcs_tSCLK_NCS_write); // tSCLK-NCS for write operation
  this->com_end();
  delayMicroseconds(mcs_tSWW - mcs_tSCLK_NCS_write); // Could be shortened, but is looks like a safe lower bound 
}

extern "C"
{
  extern unsigned short firmware_length;
  extern prog_uchar firmware_data[];
};

void adns::upload_firmware()
{  
  // send the firmware to the chip, cf p.18 of the datasheet
//  Serial.println("Uploading firmware...");
  // set the configuration_IV register in 3k firmware mode
  this->write_reg(REG_Configuration_IV, 0x02); // bit 1 = 1 for 3k mode, other bits are reserved 
  
  // write 0x1d in SROM_enable reg for initializing
  this->write_reg(REG_SROM_Enable, 0x1d); 
  
  // wait for more than one frame period
  delay(10); // assume that the frame rate is as low as 100fps... even if it should never be that low
  
  // write 0x18 to SROM_enable to start SROM download
  this->write_reg(REG_SROM_Enable, 0x18); 
  
  // write the SROM file (=firmware data) 
  this->com_begin();
  SPI.transfer(REG_SROM_Load_Burst | 0x80); // write burst destination adress
  delayMicroseconds(15);
  
  // send all bytes of the firmware
  unsigned char c;
  for(int i = 0; i < firmware_length; i++)
  { 
    c = (unsigned char)pgm_read_byte(firmware_data + i);
    SPI.transfer(c);
    delayMicroseconds(15);
  }

  this->com_end();
}

void adns::dispRegisters(void)
{
  int oreg[7] = 
  {
    0x00,0x3F,0x2A,0x02  
  };
  char* oregname[] = 
  {
    "Product_ID","Inverse_Product_ID","SROM_Version","Motion"  
  };
  byte regres;

  int rctr=0;
  for(rctr=0; rctr<4; rctr++)
  {
    regres = this->read_reg(oreg[rctr]);
    Serial.println("---");
    Serial.println(oregname[rctr]);
    Serial.println(oreg[rctr],HEX);
    Serial.println(regres,BIN);  
    Serial.println(regres,HEX);  
    delay(1);
  }
}
  
 
int adns::read_x()
{
  unsigned char l = this->read_reg(REG_Delta_X_L);
  signed char h = this->read_reg(REG_Delta_X_H);
  
  return (((int)h) << 8) | (unsigned int)l;  
}

int adns::read_y()
{  
  unsigned char l = this->read_reg(REG_Delta_Y_L);
  signed char h = this->read_reg(REG_Delta_Y_H);
  
  return (((int)h) << 8) | (unsigned int)l;  
}


