#include <AD9850.h>
#define pulse(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW);}

AD9850::AD9850(char w_clk, char fq_ud, char d7)
    : W_CLK(w_clk), FQ_UD(fq_ud), D7(d7) {
    frequency = 0;
    phase = 0;
    pinMode(W_CLK, OUTPUT);
    pinMode(FQ_UD, OUTPUT);
    pinMode(D7, OUTPUT);
    pulse(W_CLK);
    pulse(FQ_UD);
}

void AD9850::update() {
    uint32_t f = frequency;
    for (int i = 0; i < 32; i++, f >>= 1) {
        digitalWrite(D7, f & (uint32_t)0x00000001);
        pulse(W_CLK);
    }
    uint8_t p = phase;
    for (int i = 0; i < 8; i++, p >>= 1) {
        digitalWrite(D7, p & (uint8_t)0x01);
        pulse(W_CLK);
    }
    pulse(FQ_UD);
}

void AD9850::setfreqposition(int position)
{
    frequency=frequencys[position];
    update();
}

bool AD9850::initfreq(double freq,int position)
{
    if(position>15)return false;
    frequencys[position] = freq * 4294967296.0 / EX_CLK;
    return true;
}

void AD9850::setfreq(double f) {
    frequency = f * 4294967296.0 / EX_CLK;
    update();
}
void AD9850::setphase(uint8_t p) {
    phase = p << 3;
    update();
}

void AD9850::down() {
    pulse(FQ_UD);
    uint8_t p = 0x04;
    for (int i = 0; i < 8; i++, p >>= 1) {
        digitalWrite(D7, p & (uint8_t)0x01);
        pulse(W_CLK);
    }
    pulse(FQ_UD);
}

void AD9850::up() { update(); }
