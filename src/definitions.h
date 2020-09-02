#define VERSION      "1.00.beta"
#define TEST_PIN      5

unsigned short volts, amperes, fwd, rev;
int psu_temp, tx_temp;

byte w_temp;

unsigned long time_now;

bool once_flag = false;
