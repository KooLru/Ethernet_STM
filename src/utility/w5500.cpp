/*
 * Copyright (c) 2010 by WIZnet <support@wiznet.co.kr>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <string.h>

#include "utility/w5100.h"
#if defined(W5500_ETHERNET_SHIELD)

// W5500 controller instance
W5500Class W5100;

void W5500Class::init(void)
{
  delay(1000);

#if defined(ARDUINO_ARCH_AVR)
  initSS();
  ETHERNET_SPI.begin();
#elif defined (__STM32F1__)
  pinMode(ETHERNET_SPI_CS, OUTPUT);
  ETHERNET_SPI.begin();
  ETHERNET_SPI.setBitOrder(MSBFIRST);
  ETHERNET_SPI.setDataMode(SPI_MODE0);
  ETHERNET_SPI.setClockDivider(SPI_CLOCK_DIV8);
  //pinMode(ETHERNET_SPI_CS, OUTPUT);
#else
	pinMode(ETHERNET_SPI_CS, OUTPUT);
  ETHERNET_SPI.begin(ETHERNET_SPI_CS);
  // Set clock to 4Mhz (W5100 should support up to about 14Mhz)
  //ETHERNET_SPI.setClockDivider(ETHERNET_SPI_CS, 21);
  //ETHERNET_SPI.setClockDivider(ETHERNET_SPI_CS, 6); // 14 Mhz, ok  
//  ETHERNET_SPI.setClockDivider(ETHERNET_SPI_CS, 3); // 28 Mhz, ok 
  ETHERNET_SPI.setClockDivider(ETHERNET_SPI_CS, 2); // 42 Mhz, ok 
  ETHERNET_SPI.setDataMode(ETHERNET_SPI_CS, SPI_MODE0);
#endif
  write(0x00, 0x05, 128); // Software reset the W5500 chip
  for (int i=0; i<MAX_SOCK_NUM; i++) {
    uint8_t cntl_byte = (0x0C + (i<<5));
    write( 0x1E, cntl_byte, RXBUF_SIZE); //0x1E - Sn_RXBUF_SIZE
    write( 0x1F, cntl_byte, TXBUF_SIZE); //0x1F - Sn_TXBUF_SIZE
  }
}

uint16_t W5500Class::getTXFreeSize(SOCKET s)
{
    uint16_t val=0, val1=0;
    do {
        val1 = readSnTX_FSR(s);
        if (val1 != 0)
            val = readSnTX_FSR(s);
    } 
    while (val != val1);
    return val;
}

uint16_t W5500Class::getRXReceivedSize(SOCKET s)
{
    uint16_t val=0,val1=0;
    do {
        val1 = readSnRX_RSR(s);
        if (val1 != 0)
            val = readSnRX_RSR(s);
    } 
    while (val != val1);
    return val;
}

void W5500Class::send_data_processing(SOCKET s, const uint8_t *data, uint16_t len)
{
  // This is same as having no offset in a call to send_data_processing_offset
  send_data_processing_offset(s, 0, data, len);

}

void W5500Class::send_data_processing_offset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{

    uint16_t ptr = readSnTX_WR(s);
    uint8_t cntl_byte = (0x14+(s<<5));
    ptr += data_offset;
    write(ptr, cntl_byte, data, len);
    ptr += len;
    writeSnTX_WR(s, ptr);
    
}

void W5500Class::recv_data_processing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek)
{
    uint16_t ptr;
    ptr = readSnRX_RD(s);

    read_data(s, ptr, data, len);
    if (!peek)
    {
        ptr += len;
        writeSnRX_RD(s, ptr);
    }
}

void W5500Class::read_data(SOCKET s, volatile uint16_t src, volatile uint8_t *dst, uint16_t len)
{
    uint8_t cntl_byte = (0x18+(s<<5));
    read((uint16_t)src , cntl_byte, (uint8_t *)dst, len);
}

uint8_t W5500Class::write(uint16_t _addr, uint8_t _cb, uint8_t _data)
{
#if defined(ARDUINO_ARCH_AVR)
    setSS();  
    ETHERNET_SPI.transfer(_addr >> 8);
    ETHERNET_SPI.transfer(_addr & 0xFF);
    ETHERNET_SPI.transfer(_cb);
    ETHERNET_SPI.transfer(_data);
    resetSS();
#elif defined(__STM32F1__)
  digitalWrite(ETHERNET_SPI_CS, LOW);
  ETHERNET_SPI.transfer(_addr >> 8);
  ETHERNET_SPI.transfer(_addr & 0xFF);
  ETHERNET_SPI.transfer(_cb);
  ETHERNET_SPI.transfer(_data);
  digitalWrite(ETHERNET_SPI_CS, HIGH);
#else
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr >> 8, SPI_CONTINUE);
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr & 0xFF, SPI_CONTINUE);
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _cb, SPI_CONTINUE);
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _data);
#endif    
    return 1;
}

uint16_t W5500Class::write(uint16_t _addr, uint8_t _cb, const uint8_t *_buf, uint16_t _len)
{
#if defined(ARDUINO_ARCH_AVR)
    setSS();
    ETHERNET_SPI.transfer(_addr >> 8);
    ETHERNET_SPI.transfer(_addr & 0xFF);
    ETHERNET_SPI.transfer(_cb);
    for (uint16_t i=0; i<_len; i++){
        ETHERNET_SPI.transfer(_buf[i]);
    }
    resetSS();
#elif defined(__STM32F1__)
  digitalWrite(ETHERNET_SPI_CS, LOW);
	ETHERNET_SPI.transfer( _addr >> 8);
	ETHERNET_SPI.transfer( _addr & 0xFF);
  ETHERNET_SPI.transfer(_cb);
  for (uint16_t i=0; i<_len; i++){
        ETHERNET_SPI.transfer(_buf[i]);
  }
  digitalWrite(ETHERNET_SPI_CS, HIGH);
#else
  uint16_t i;
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr >> 8, SPI_CONTINUE);
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr & 0xFF, SPI_CONTINUE);
  ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _cb, SPI_CONTINUE);
  for (i=0; i<_len-1; i++){
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _buf[i], SPI_CONTINUE);
  }
	ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _buf[i]);

#endif    
    return _len;
}

uint8_t W5500Class::read(uint16_t _addr, uint8_t _cb)
{
#if defined(ARDUINO_ARCH_AVR)
    setSS();
    ETHERNET_SPI.transfer(_addr >> 8);
    ETHERNET_SPI.transfer(_addr & 0xFF);
    ETHERNET_SPI.transfer(_cb);
    uint8_t _data = ETHERNET_SPI.transfer(0);
    resetSS();
#elif defined(__STM32F1__)
  digitalWrite(ETHERNET_SPI_CS, LOW);
  ETHERNET_SPI.transfer( _addr >> 8);
  ETHERNET_SPI.transfer(_addr & 0xFF);
  ETHERNET_SPI.transfer(_cb);
  uint8_t _data = ETHERNET_SPI.transfer(0);
  digitalWrite(ETHERNET_SPI_CS, HIGH);
#else
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr >> 8, SPI_CONTINUE);
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr & 0xFF, SPI_CONTINUE);
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _cb, SPI_CONTINUE);
    uint8_t _data = ETHERNET_SPI.transfer(ETHERNET_SPI_CS, 0);
#endif    
    return _data;
}

uint16_t W5500Class::read(uint16_t _addr, uint8_t _cb, uint8_t *_buf, uint16_t _len)
{ 
#if defined(ARDUINO_ARCH_AVR)
    setSS();
    ETHERNET_SPI.transfer(_addr >> 8);
    ETHERNET_SPI.transfer(_addr & 0xFF);
    ETHERNET_SPI.transfer(_cb);
    for (uint16_t i=0; i<_len; i++){
        _buf[i] = ETHERNET_SPI.transfer(0);
    }
    resetSS();
#elif defined(__STM32F1__)
  digitalWrite(ETHERNET_SPI_CS, LOW);
	ETHERNET_SPI.transfer(_addr >> 8);
	ETHERNET_SPI.transfer(_addr & 0xFF);
  ETHERNET_SPI.transfer(_cb);
  for (uint16_t i=0; i<_len; i++){
        _buf[i] = ETHERNET_SPI.transfer(0);
  }
  digitalWrite(ETHERNET_SPI_CS, HIGH);
#else
    uint16_t i;
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr >> 8, SPI_CONTINUE);
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _addr & 0xFF, SPI_CONTINUE);
    ETHERNET_SPI.transfer(ETHERNET_SPI_CS, _cb, SPI_CONTINUE);
  for (i=0; i<_len-1; i++){
    _buf[i] = ETHERNET_SPI.transfer(ETHERNET_SPI_CS, 0, SPI_CONTINUE);
  }
    _buf[_len-1] = ETHERNET_SPI.transfer(ETHERNET_SPI_CS, 0);
	    

#endif    
    return _len;
}

void W5500Class::execCmdSn(SOCKET s, SockCMD _cmd) {
    // Send command to socket
    writeSnCR(s, _cmd);
    // Wait for command to complete
    while (readSnCR(s))
    ;
}
#endif
