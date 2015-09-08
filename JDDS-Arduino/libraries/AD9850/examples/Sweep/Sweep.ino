#include <AD9850.h>

AD9850 ad(5, 6, 7); // w_clk, fq_ud, d7

void setup() {}

// sweep form 1MHz to 10MHz
void loop() {
  for (uint32_t i = 1e3; i < 1e4; i++) {
    ad.setfreq(i * 1e3);
  }
}
