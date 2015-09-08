#include <Slip.h>
Slip slip;

void slipcallback(uchar *buf,uchar len)
{
  slip.sendpacket(buf,len);
}

void setup()
{
slip.setCallback(slipcallback);
Serial.begin(9600);
}

void loop()
{
slip.proc();
}
