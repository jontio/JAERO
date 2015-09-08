#ifndef Slip_h
#define Slip_h


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

typedef unsigned char uchar;

//#include <stdlib.h>

/* SLIP special character codes
 */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDE    /* ESC ESC_ESC means ESC data byte */

#define SLIP_MAXINBUFFERSIZE      255

class Slip
{
public:
    Slip();
    ~Slip();
    void proc();
    bool sendpacket(uchar *buf,uchar len);
    void (*slipReadCallback)(uchar *,uchar) = NULL;
    void setCallback(void (*callback)(uchar *,uchar)) __attribute__((always_inline))
    {
        slipReadCallback = callback;
    }
private:
    uchar lastbyte;
    void handleByte(uchar c);
    uchar inbuffer[SLIP_MAXINBUFFERSIZE];
    uchar inbuffer_ptr=0;
};


#endif
