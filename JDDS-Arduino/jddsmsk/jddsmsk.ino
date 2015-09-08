#include <Slip.h>   //http://jontio.zapto.org/hda1/SLIP_library_for_Arduino.html
#include <AD9850.h> //modified version of https://code.google.com/p/ad9850-arduino/downloads/list
#include <TimerOne.h>

#define JPKT_ON              0x01    /* turn on/off dds                                 */
#define JPKT_FREQ            0x02    /* set freq of dds                                 */
#define JPKT_PHASE           0x03    /* set phase of dds                                */
#define JPKT_Data            0x05    /* Data to arduino                                 */
#define JPKT_REQ             0x06    /* packet from arduino saying it wants more data   */
#define JPKT_DATA_ACK        0x07    /* ACK recept for given data                       */
#define JPKT_DATA_NACK       0x08    /* NACK recept for given data (no room)            */
#define JPKT_SYMBOL_PERIOD   0x09    /* set symbol period in microseconds               */
#define JPKT_PILOT_ON        0x0A    /* turn on/off dds single frequency                */
#define JPKT_SET_FREQS       0x0B    /* set multi freq for later referencing            */
#define JPKT_GEN_ACK         0x0C    /* general ack                                     */
#define JPKT_GEN_NAK         0x0D    /* general nack                                    */

Slip slip;
AD9850 ad(5, 6, 7); // w_clk, fq_ud, d7. what pins the AD9850 are attached to

int led = 13;
long symbol_period=32000;//default symbol period is 32000us == 31.25 bps
bool ad_on=false;
bool bufferempty=false;
bool bufferlow_msg_sent_lock=false;
int symbolbufferavailable_last_result=0;

//define size of symbol buffer and how often we request more data.
//BYTES_PER_DATA_REQ<=255
//For slip v1.1 SLIP_MAXINBUFFERSIZE <= 255
//SYMBOLBUFFER_SZ>BYTES_PER_DATA_REQ else we will underflow
//when symbolbuffer has at least BYTES_PER_DATA_REQ bytes free a data request will be made (note these are overridden below)
#define BYTES_PER_DATA_REQ      SLIP_MAXINBUFFERSIZE
#define SYMBOLBUFFER_SZ         BYTES_PER_DATA_REQ*3

//these override how often and how much data is reqed. This is to keep data txfer intervals constant for different bit rates
uchar bytestoreq=BYTES_PER_DATA_REQ;
int minfreespacebeforereq=BYTES_PER_DATA_REQ;

uchar symbolbuffer[SYMBOLBUFFER_SZ];
int symbolbuffer_tail_ptr_tmp=0;
int symbolbuffer_tail_ptr=0;
int symbolbuffer_head_ptr_tmp=0;
int symbolbuffer_head_ptr=0;

//for binary type modes like 2msk for speed only use 1 memory bit per symbol. this brakes the previous format that used 4 bits
uchar symbolbuffer_bitptr=0;
#define symbolbuffer_bitptr_step_sz 1

void slipcallback(uchar *buf,uchar len)
{
  bufferlow_msg_sent_lock=false;
  switch(buf[0])//switch on packet type
  {
    case JPKT_Data:
      if(symbolbufferavailable()>=(len-1))//if we have space then add to symbol buffer
      {
        
        symbolbuffer_head_ptr_tmp=symbolbuffer_head_ptr;
        for(int i=0;i<(len-1);i++)
        {
          symbolbuffer[symbolbuffer_head_ptr_tmp]=buf[1+i];
          symbolbuffer_head_ptr_tmp++;symbolbuffer_head_ptr_tmp%=SYMBOLBUFFER_SZ;
        }
        cli();
        symbolbuffer_head_ptr=symbolbuffer_head_ptr_tmp;
        sei();
        
        //marsked out. gui doesnt do anything with the ack and just wastes capacity and CPU time
        //uchar c=JPKT_DATA_ACK;
        //slip.sendpacket(&c,1);
         
      }
       else
       {
         uchar c=JPKT_DATA_NACK;
         slip.sendpacket(&c,1);
       }
    break;
    case JPKT_SET_FREQS:
      if((!((len-1)%4))&&(!ad_on))//DDS must be off to change fsk freqs
      {
        int k=0;
        for(int i=0;i<(len-1);i+=4)
        {
          ad.initfreq(*((double*)&buf[1+i]),k);
          k++;
        }
        slip.sendpacket(buf,len);//echo back
      }
    break;
      case JPKT_SYMBOL_PERIOD:
      if(len==(1+4))
      {
        symbol_period=*((long*)&buf[1]);
        
        long bytesleft=(2000000l/symbol_period)/8l;//two secs of buffer
        minfreespacebeforereq=(SYMBOLBUFFER_SZ-bytesleft);
        bytestoreq=bytesleft/2;//one sec of data
        
        if(ad_on)
        {
          Timer1.stop();
          //set symbol timer
          Timer1.initialize(symbol_period); // symbol period in microseconds
          Timer1.start();
          Timer1.attachInterrupt( symbolTimerFSKIsr ); // attach the service routine here
          sei();//is disabled in some versions of timer1 lib
        }
        slip.sendpacket(buf,len);//echo back
      }
    break;
    case JPKT_ON:
      if(len==(1+1))
      {
        
        //turning off or on, resets the symbol buffer bit pointer and stops the symbol timer
        Timer1.stop();
        symbolbuffer_bitptr=0;
        
        if(buf[1]==0)
        {
          ad.down();//turn off
          ad_on=false;
          symbolbuffer_tail_ptr=0;
          symbolbuffer_head_ptr=0;
          digitalWrite(led, LOW);
          buf[0]=JPKT_GEN_ACK;
          buf[1]=JPKT_ON;
          buf[2]=0;
          slip.sendpacket(buf,3);
          break;
        }
          else if(buf[1]==2)//turn on FSK
          {
             digitalWrite(led, HIGH);
             ad.setfreqposition(0);//turn on and set freq
             ad_on=true;

             //set symbol timer
             Timer1.initialize(symbol_period); // symbol period in microseconds
             Timer1.start();
             Timer1.attachInterrupt( symbolTimerFSKIsr ); // attach the service routine here
             sei();//is disabled in some versions of timer1 lib
             
             buf[0]=JPKT_GEN_ACK;
             buf[1]=JPKT_ON;
             buf[2]=2;
             slip.sendpacket(buf,3);
             break;            
          }
        buf[0]=JPKT_GEN_NAK;
        buf[2]=buf[1];
        buf[1]=JPKT_ON;
        slip.sendpacket(buf,3);
      }
    break;
    case JPKT_PILOT_ON:
      if(len==(1+0))
      {
        //stops the symbol timer
        Timer1.stop();
        symbolbuffer_bitptr=0;
        bufferempty=false;
        //turn on
        digitalWrite(led, HIGH);
        ad.setfreqposition(0);
        ad_on=true;
        slip.sendpacket(buf,len);//echo back
      }
    break;    
    
    
  } 
}

void symbolTimerFSKIsr()
{
  
  //if buffer is empty then we stop
  if(symbolbuffer_head_ptr == symbolbuffer_tail_ptr)
  {
    bufferempty=true;
    return;
  }
  
  //load next freq into dds
  ad.setfreqposition(((symbolbuffer[symbolbuffer_tail_ptr]>>symbolbuffer_bitptr)&0x01));
  symbolbuffer_bitptr+=symbolbuffer_bitptr_step_sz;symbolbuffer_bitptr%=8;
  if(!symbolbuffer_bitptr)
  {
    symbolbuffer_tail_ptr++;symbolbuffer_tail_ptr%=SYMBOLBUFFER_SZ;
  }
  
}

int symbolbufferavailable()//space available in the symbol buffer
{
  cli();
  symbolbuffer_tail_ptr_tmp=symbolbuffer_tail_ptr;
  sei();
  if (symbolbuffer_head_ptr >= symbolbuffer_tail_ptr_tmp) return (SYMBOLBUFFER_SZ + symbolbuffer_tail_ptr_tmp - 1 - symbolbuffer_head_ptr) ;
   else return (symbolbuffer_tail_ptr_tmp - symbolbuffer_head_ptr - 1);
}

void setup()
{
  ad.down();
  slip.setCallback(slipcallback);
  Serial.begin(19200);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
}

void loop()
{
  slip.proc();

  if(bufferempty)//turn off if buffer is empty
  {
    Timer1.stop();
    symbolbuffer_tail_ptr=0;
    symbolbuffer_head_ptr=0;
    symbolbuffer_bitptr=0;
    ad.down();//turn off
    ad_on=false;
    bufferempty=false;
    bufferlow_msg_sent_lock=false;
    uchar c[2];
    c[0]=JPKT_ON;
    c[1]=0;
    slip.sendpacket(c,2);
    digitalWrite(led, LOW);
  }
   else if((ad_on)&&(!bufferlow_msg_sent_lock)&&(symbolbufferavailable()>=minfreespacebeforereq))//if buffer is low ask for more data and wait for a responce.
   {
     uchar c[2];
     c[0]=JPKT_REQ;
     c[1]=bytestoreq;
     slip.sendpacket(c,2);
     bufferlow_msg_sent_lock=true;
   }
   
}
