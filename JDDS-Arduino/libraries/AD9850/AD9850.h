/*
 * ------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file. As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Poul-Henning Kamp
 * ------------------------------------------------------------------------
 */
#ifndef AD9850_H
#define AD9850_H
#include <Arduino.h>
#include <stdint.h>
#define EX_CLK 125.0e6
// change if ad9850 is connected to other frequency crystal oscillator

class AD9850
{
    private:
        const char W_CLK; // word load clock pin
        const char FQ_UD; // frequency update pin
        const char D7; // serial input pin
        uint32_t frequency; // delta phase
        uint8_t phase; // phase offset

        //jonti
        uint32_t frequencys[16];

        void update();
    public:
        AD9850(char w_clk, char fq_ud, char d7);
        /* NOTE: For device start-up in serial mode,
        hardwire pin 2 at 0, pin 3 at 1, and pin 4 at 1 */
        bool initfreq(double freq,int position);
        void setfreqposition(int position);
        void setfreq(double f);
        // set frequency in Hz
        void setphase(uint8_t p);
        // for flexibility, p is an int value and 0 <= p <= 32,
        // as input, 360 degree devide into 32 parts,
        // you will get phase increments for 360/32*p degree
        void down();
        // power down
        void up();
        // power on
};
#endif
