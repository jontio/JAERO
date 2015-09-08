#include "Slip.h"

Slip::Slip()
{

}

Slip::~Slip()
{

}

void Slip::handleByte(uchar c)
{

    if(c==SLIP_END)
    {
        if(inbuffer_ptr>0)slipReadCallback(inbuffer,inbuffer_ptr);
        inbuffer_ptr=0;
        lastbyte=c;
        return;
    }

    if(inbuffer_ptr>=SLIP_MAXINBUFFERSIZE)inbuffer_ptr=0;

    if(lastbyte==SLIP_ESC)
    {
        switch(c)
        {
        case SLIP_ESC_END:
            inbuffer[inbuffer_ptr]=SLIP_END;
            inbuffer_ptr++;
            break;
        case SLIP_ESC_ESC:
            inbuffer[inbuffer_ptr]=SLIP_ESC;
            inbuffer_ptr++;
            break;
        }
        lastbyte=c;
        return;
    }

    if(c!=SLIP_ESC)
    {
        inbuffer[inbuffer_ptr]=c;
        inbuffer_ptr++;
    }

    lastbyte=c;
}

bool Slip::sendpacket(uchar *buf,uchar len)
{
    if(Serial.availableForWrite()<(1+2*((int)len)))return false;//the esc chars will change the size a bit but who knows exacly. in hardwareserial for uno sz==64 --> 31 is the max sz of tx pkt to be safe.
    //Serial.write(SLIP_END);
    for(int i=0;i<len;i++)
    {
        switch((uchar)buf[i])
        {
        case SLIP_END:
            Serial.write(SLIP_ESC);
            Serial.write(SLIP_ESC_END);
            break;
        case SLIP_ESC:
            Serial.write(SLIP_ESC);
            Serial.write(SLIP_ESC_ESC);
            break;
        default:
            Serial.write(buf[i]);
        }
    }
    Serial.write(SLIP_END);
    return true;
}

void Slip::proc()
{
    while (Serial.available() > 0)
    {
        handleByte(Serial.read());
    }
}
