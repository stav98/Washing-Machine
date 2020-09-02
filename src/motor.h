#include <PID_v1.h>

#define ZERO_CROSS    33
#define TRIAC_GATE    32
#define MOTOR_FWD     4
#define MOTOR_REV     16
#define TACHO_PIN     17

#define motorFWD()  {digitalWrite(MOTOR_FWD, HIGH); digitalWrite(MOTOR_REV, LOW); delay(1);}
#define motorREV()  {digitalWrite(MOTOR_FWD, LOW); digitalWrite(MOTOR_REV, HIGH); delay(1);}
#define motorSTOP() {digitalWrite(MOTOR_FWD, LOW); digitalWrite(MOTOR_REV, LOW);}

#define TACHOPULSES       8
#define AVERAGE_FACTOR    4

volatile boolean zero_cross;
volatile short dim, dim_cnt, motor_speed;
//volatile unsigned long tachotime;
unsigned long lastflash, lastcounttime = 0, lastpiddelay = 0;
unsigned long plisidelay = 0;
volatile unsigned short avg_rpm, count;
unsigned short rpm, lastcount;
unsigned short target_rpm, tempcounter = 100;
bool start_flag = false;

const short rpmcorrection = 4;

double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double sKp=0.1, sKi=0.2, sKd=0;
double Kp=0.25, Ki=1, Kd=0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

//motor_speed  0 - 430  Το μοτέρ αρχίζει να γυρίζει με 60 και με 390 φτάνει 25000 RPM χωρίς φορτίο
//dim = 490 - motor_speed δηλ. για Full στροφές η τιμή είναι 60, για να ξεκινήσει να βγάζει κάτι είναι 490
const short minoutputlimit = 60; //50;      // limit of PID output
const short maxoutputlimit = 490; //400;     // limit of PID output
const short mindimminglimit = 60;     // the shortest delay before triac fires
const short maxdimminglimit = 480;
const short sampleRate = 1; 

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void endTimer() 
{
 timerEnd(timer);
 timer = NULL; 
}

void IRAM_ATTR onTimer() 
     {
      portENTER_CRITICAL_ISR(&timerMux);
      if (zero_cross) 
         {    
          if (dim_cnt > dim  + 3) //Πλάτος παλμού 3 * 20 = 60μsec
             {
              digitalWrite(TRIAC_GATE, LOW);
              dim_cnt = 0;  //μηδένισε μετρητή                    
              zero_cross = false; //μην ξαναμπείς εδώ
             }
          else if (dim_cnt > dim) 
             {                     
              digitalWrite(TRIAC_GATE, HIGH); //Άνοδος παλμού       
              dim_cnt++;
             }
          else 
              dim_cnt++; //Αύξησε τον μετρητή κατά ένα βήμα                                                     
         }
      portEXIT_CRITICAL_ISR(&timerMux);  
     }

void startTimer() 
{
  timer = timerBegin(0, 80, true); //80MHz : 80 = 1MHz  prescaler
  timerAttachInterrupt(timer, &onTimer, true);
  //100Hz = 10msec αν θέλω 500 βήματα έντασης 10msec / 500 = 0,02msec ή 20μsec ανά βήμα
  timerAlarmWrite(timer, 20, true); //1MHz : 20 = 50000Hz δηλ. βήμα 20μsec
  timerAlarmEnable(timer);
}
     
void IRAM_ATTR zero_cross_ISR()
     {
      digitalWrite(TRIAC_GATE, LOW);
      zero_cross = true; //Μόλις έγινε ανίχνευση αιχμής zero-cross
      dim_cnt = 0;  //μηδένισε μετρητή    
     }

void IRAM_ATTR tacho_ISR()
     {
      count++;
      unsigned long tachotime = micros() - lastflash;
      unsigned long rpm1 = 60000000 / tachotime;
      rpm = rpm1 / TACHOPULSES;
      rpm += rpmcorrection;
      avg_rpm = (AVERAGE_FACTOR * avg_rpm + rpm) / (AVERAGE_FACTOR + 1);
      lastflash = micros();
     }

void init_motor()
     {
      pinMode(ZERO_CROSS, INPUT_PULLUP);
      pinMode(TRIAC_GATE, OUTPUT);
      pinMode(MOTOR_FWD, OUTPUT);
      pinMode(MOTOR_REV, OUTPUT);
      motorFWD();
      attachInterrupt(ZERO_CROSS, zero_cross_ISR, RISING);
      startTimer();
      Input = 1200;                        // asiign initial value for PID
      Setpoint = 1200;                     // asiign initial value for PID
      //Εκκίνηση του ελεγκτή PID
      myPID.SetMode(AUTOMATIC);
      myPID.SetOutputLimits(minoutputlimit, maxoutputlimit);
      myPID.SetSampleTime(sampleRate);    // Sets the sample rate
     }

void motor_control()
     {
      //Ομαλή εκκίνηση
      if (start_flag) 
         {
          myPID.SetTunings(sKp, sKi, sKd);        //Σταθερές PID για την εκκίνηση
          short i = (target_rpm - tempcounter);  
          for (short j = 1; j <= i; j++) 
              {
               Input = avg_rpm;
               Setpoint = tempcounter;  //Ξεκινάει από 100
               myPID.Compute();
               dim = map(Output, minoutputlimit, maxoutputlimit, maxoutputlimit, minoutputlimit); // inverse the output
               dim = constrain(dim, mindimminglimit, maxdimminglimit);
               tempcounter++;
               delayMicroseconds(100); //Καθυστέρηση για το επόμενο βήμα
              }
          //Αν έφτασε τις επιθυμητές στροφές
          if (tempcounter >= target_rpm) 
             {
              lastcounttime = millis();
              lastpiddelay = millis();
              start_flag = false; //Μην ξαναμπείς εδώ
              //run_flag = true;
              tempcounter = 100; //Για την επόμενη εκκίνηση
             }
         }
      //Κανονική λειτουργία
      if (!start_flag) 
         {
          unsigned long piddelay = millis();
          //Περίμενε 1sec μέχρι να αλλάξει σταθερές PID
          if ((piddelay - lastpiddelay) > 1000) 
             {   
              myPID.SetTunings(Kp, Ki, Kd);    //Άλλαξε σταθερές PID για κανονική λειτουργία
              lastpiddelay = millis();
             }
          Input = avg_rpm;
          Setpoint = target_rpm;
          myPID.Compute();
          dim = map(Output, minoutputlimit, maxoutputlimit, maxoutputlimit, minoutputlimit); // inverse the output
          dim = constrain(dim, mindimminglimit, maxdimminglimit);
          //dim = 390; //Δοκιμή ανοιχτού βρόχου
         }
      //Αν περάσει 1sec μηδένισε τον count που αυξάνει μέσα στο tacho_ISR. 
      unsigned long counttime = millis();
      if (counttime - lastcounttime >= 1000) 
         {
          lastcount = count;
          count = 0;
          lastcounttime = millis();
         }
      //Αν είναι μηδέν σημαίνει ότι δεν έγινε interrupt, επομένως δεν περιστρέφεται
      if (count == 0 && lastcount == 0)
          avg_rpm = 0;
     }

typedef void (*FuncPtr)(void);

typedef struct _motor_instr
        {
         byte state;
         unsigned long delay_time;
         FuncPtr s_func;
        } motor_instr;

FuncPtr FPtr;

void toggle_FWD_REV(void);
void start_turn_slow(void);
void stop_turn_slow(void);
void stop_stator(void);


motor_instr instr[] = {{0,  1000, toggle_FWD_REV},
                       {1,  9000, start_turn_slow},
                       {2, 10000, stop_turn_slow},
                       {3, 16000, stop_stator},
                       {4,     0, 0}};

void plisi_turn()
     {
      static byte i = 0, state = 0;
      static unsigned long w_time = 0;
      if (millis() - w_time < instr[i].delay_time)
         {
          if (state == instr[i].state)
             {
              FPtr = instr[i].s_func;
              FPtr();
              state++;
             }
         }
      else
          i++;
      if (instr[i].delay_time == 0)
         {
          w_time = millis();
          i = 0;
          state = 0;
         }
     }

void toggle_FWD_REV()
     {
      static bool dir = false;
      if (dir)
         {
          //Serial.println("FWD");
          motorFWD();
          lcd.img(motor_left, 91, 48, true);
          dir = false;
         }
      else
         {
          //Serial.println("REV");
          motorREV();
          lcd.img(motor_right, 91, 48, true);
          dir = true;
         }
      delay(20);
      avg_rpm = 0;  
     }

void start_turn_slow()
     {
      target_rpm = 1200;
      start_flag = true;
     }

void stop_turn_slow()
     {
      target_rpm = 0;
      start_flag = true;
     }

void stop_stator()
     {
      motorSTOP();
      lcd.img(motor_idle, 91, 48);
     }
