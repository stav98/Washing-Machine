#define VERSION      "1.00.beta"
#define TEST_PIN      5
#define VALVE1        26
#define VALVE2        27
#define PUMP          23
#define HEATER        22
#define DOOR_LOCK     21
#define WATER_L1      39

#define waterinp1()      {digitalWrite(VALVE1, HIGH); digitalWrite(VALVE2, LOW); delay(10);}
#define waterinp2()      {digitalWrite(VALVE1, LOW); digitalWrite(VALVE2, HIGH); delay(10);}
#define waterinp3()      {digitalWrite(VALVE1, HIGH); digitalWrite(VALVE2, HIGH); delay(10);}
#define waterinp_none()  {digitalWrite(VALVE1, LOW); digitalWrite(VALVE2, LOW); delay(10);}
#define pump_on()        {digitalWrite(PUMP, HIGH); delay(10);}
#define pump_off()       {digitalWrite(PUMP, LOW); delay(10);}
#define heat_on()        {digitalWrite(HEATER, HIGH); delay(10);}
#define heat_off()       {digitalWrite(HEATER, LOW); delay(10);}
#define door_lock()      {digitalWrite(DOOR_LOCK, HIGH); delay(10);}
#define door_unlock()    {digitalWrite(DOOR_LOCK, LOW); delay(10);}

unsigned short volts, amperes, fwd, rev;
int psu_temp; //tx_temp;

byte w_temp, dest_w_temp;

bool AUX1, CYCLE_OK, stop_flag = false;
byte state = 0, param1, param2;
