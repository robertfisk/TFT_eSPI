////////////////////////////////////////////////////
        //       TFT_eSPI generic driver functions        //
        ////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////////////

// Select the SPI port to use
#ifdef TFT_SPI_PORT
  SPIClass& spi = TFT_SPI_PORT;
#else
  SPIClass& spi = SPI;
#endif

////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           tft_Read_8
** Description:             Bit bashed SPI to read bidirectional SDA line
***************************************************************************************/
uint8_t TFT_eSPI::tft_Read_8(void)
{
  uint8_t  ret = 0;

  for (uint8_t i = 0; i < 8; i++) {  // read results
    ret <<= 1;
    SCLK_L;
    if (digitalRead(TFT_MOSI)) ret |= 1;
    SCLK_H;
  }

  return ret;
}

/***************************************************************************************
** Function name:           beginSDA
** Description:             Detach SPI from pin to permit software SPI
***************************************************************************************/
void TFT_eSPI::begin_SDA_Read(void)
{
  // Release configured SPI port for SDA read
  spi.end();
}

/***************************************************************************************
** Function name:           endSDA
** Description:             Attach SPI pins after software SPI
***************************************************************************************/
void TFT_eSPI::end_SDA_Read(void)
{
  // Configure SPI port ready for next TFT access
  spi.begin();
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // #if defined (TFT_SDA_READ)
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
#if defined (RPI_WRITE_STROBE)  // For RPi TFT with write strobe                      
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32 or STM32 RPi TFT
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  if(len) { tft_Write_16(color); len--; }
  while(len--) {WR_L; WR_H;}
}

/***************************************************************************************
** Function name:           pushPixels - for ESP32 or STM32 RPi TFT
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) {tft_Write_16S(*data); data++;}
  else while ( len-- ) {tft_Write_16(*data); data++;}
}

////////////////////////////////////////////////////////////////////////////////////////
#elif defined (SPI_18BIT_DRIVER) // SPI 18-bit colour                         
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for STM32 and 3 byte RGB display
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  // Split out the colours
  uint8_t r = (color & 0xF800)>>8;
  uint8_t g = (color & 0x07E0)>>3;
  uint8_t b = (color & 0x001F)<<3;

  while ( len-- ) {tft_Write_8(r); tft_Write_8(g); tft_Write_8(b);}
}

/***************************************************************************************
** Function name:           pushPixels - for STM32 and 3 byte RGB display
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while ( len-- ) {
      uint16_t color = *data >> 8 | *data << 8;
      tft_Write_8((color & 0xF800)>>8);
      tft_Write_8((color & 0x07E0)>>3);
      tft_Write_8((color & 0x001F)<<3);
      data++;
    }
  }
  else {
    while ( len-- ) {
      tft_Write_8((*data & 0xF800)>>8);
      tft_Write_8((*data & 0x07E0)>>3);
      tft_Write_8((*data & 0x001F)<<3);
      data++;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
#else //                   Standard SPI 16-bit colour TFT                               
////////////////////////////////////////////////////////////////////////////////////////

#define BUFFER_LENGTH_UINT16 16

/***************************************************************************************
** Function name:           pushBlock - for STM32
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len) {

  uint32_t i;
  union {
    uint16_t val;
    struct {
      uint8_t lsb;
      uint8_t msb; };
  } buffer[BUFFER_LENGTH_UINT16];

  //Fill the buffer with our color
  buffer[0].lsb = color >> 8;
  buffer[0].msb = color;
  for (i = 1; i < BUFFER_LENGTH_UINT16; i++) {
    buffer[i].val = buffer[0].val;
  }

  while (len > 0)
  {
    i = min(len, BUFFER_LENGTH_UINT16);
    spi.transfer(buffer, NULL, (i * 2));
    len = len - i;
  }
}

/***************************************************************************************
** Function name:           pushPixels - for STM32
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len) {

  uint16_t *data = (uint16_t*)data_in;

  uint32_t transferLen;
  uint32_t i;
  union {
    uint16_t val;
    struct {
      uint8_t lsb;
      uint8_t msb; };
  } buffer[BUFFER_LENGTH_UINT16];

  if (_swapBytes) {
    while (len > 0)
    {
      transferLen = min(len, BUFFER_LENGTH_UINT16);
      for (i = 0; i < transferLen; i++)
      {
        buffer[i].lsb = *data >> 8;
        buffer[i].msb = *data;
        data++;
      }
      spi.transfer(buffer, NULL, (transferLen * 2));
      len = len - transferLen;
    }
  } else {
    spi.transfer(data, NULL, (len * 2));
  }
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // End of display interface specific functions
////////////////////////////////////////////////////////////////////////////////////////


void tft_Write_8(uint8_t C) {
  spi.transfer(&C, NULL, 1);
}


void tft_Write_16(uint16_t C) {
  union {
    uint16_t val;
    struct {
      uint8_t lsb;
      uint8_t msb; };
    } t;

  //Bit order in TFT_eSPI.c is MSBFIRST, so we need to swap bytes.
  t.lsb = C >> 8;
  t.msb = C;

  spi.transfer(&t, NULL, 2);
}


void tft_Write_16S(uint16_t C) {
  spi.transfer(&C, NULL, 2);
}


void tft_Write_32(uint32_t C) {
  union {
    uint32_t val;
  struct {
    uint8_t llsb;
    uint8_t lmsb;
    uint8_t ulsb;
    uint8_t umsb;};
  } t;

  t.llsb = C >> 24;
  t.lmsb = C >> 16;
  t.ulsb = C >> 8;
  t.umsb = C;

  spi.transfer(&t, NULL, 4);
}


void tft_Write_32C(uint32_t C, uint32_t D) {
  union {
    uint32_t val;
  struct {
    uint8_t llsb;
    uint8_t lmsb;
    uint8_t ulsb;
    uint8_t umsb;};
  } t;

  t.llsb = C >> 8;
  t.lmsb = C;
  t.ulsb = D >> 8;
  t.umsb = D;

  spi.transfer(&t, NULL, 4);
}


void tft_Write_32D(uint32_t C) {
    union {
    uint32_t val;
  struct {
    uint8_t llsb;
    uint8_t lmsb;
    uint8_t ulsb;
    uint8_t umsb;};
  } t;

  t.llsb = C >> 8;
  t.lmsb = C;
  t.ulsb = C >> 8;
  t.umsb = C;

  spi.transfer(&t, NULL, 4);
}