//Η τροχαλία έχει σχέση 1:20, δηλαδή για 800 στροφές στύψιμο το μοτέρ γυρίζει με 16000.
//Το συγκεκριμένο μοντέλο είναι για max 600 RPM δηλαδή 12000 στο μοτέρ.
//Για πλύσιμο έχω 60 RPM δηλ. 1200.
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

volatile boolean zero_cross, en_flag = false;
volatile short dim, dim_cnt, motor_speed;
unsigned long lastflash, lastcounttime = 0, lastpiddelay = 0;
unsigned long tachotime;
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
const short minoutputlimit = 60;   //50;      // limit of PID output
const short maxoutputlimit = 490;  //400;     // limit of PID output
const short mindimminglimit = 60;  // the shortest delay before triac fires
const short maxdimminglimit = 480;
const short sampleRate = 1; 

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//void endTimer() 
//     {
//      timerEnd(timer);
//      timer = NULL; 
//     }

//Ασύγχρονο timer
bool async_timer(unsigned long &timerState, unsigned long timerPeriod) 
     {
      bool y;       
      if (timerState == 0) 
         {                //Δεν έχει αρχίσει να μετράει ακόμα
          timerState = millis();           // Set timerState to current time in milliseconds
          y = false;                  //Αποτέλεσμα 'δεν έχει τελειώσει' (0)
         }
      else 
         {                //Είναι ενεργός και μετράει
          if (millis() - timerState >= timerPeriod) 
             {   //Τελείωσε το μέτρημα
              y = true;                // Αποτέλεσμα = 'τελείωσε' (1)
              timerState = 0;
             }
          else  //Δεν έχει τελειώσει 
              y = false;                // Result = 'not finished' (0)
         }
      return(y);                    // Return result (1 = 'finished', 0 = 'not started' / 'not finished')
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
      en_flag = true;
     }

void IRAM_ATTR tacho_ISR()
     {
      count++;
      tachotime = 60000000 / (micros() - lastflash);
      rpm = tachotime / TACHOPULSES;
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
void stop_turn(void);
void spin_FWD(void);
void spin_REV(void);
void start_turn_bal(void);
void start_spin1(void);
void stop_motor(void);
void spin_test(void);
 
//Σχήμα πλύσης 8 ενεργό / 8 ανενεργό
//                      Α/Α, χρόνος αναμονής σε ms,       λειτουργία
motor_instr instr1[] = {{ 0,                  1000,     toggle_FWD_REV  },  //Αλλάζει φορά περιστροφής και περιμένει 1 sec (ΑΝ)
                        { 1,                  8000,     start_turn_slow },  //Γυρίζει για 8 sec (ΕΝ)
                        { 2,                  1000,     stop_turn       },  //Σταματάει και περιμένει 1 sec (ΑΝ)
                        { 3,                  6000,     stop_motor      },  //Διακόπτει ρεύμα στον στάτορα και περιμένει 6 sec (ΑΝ)
                        {99,                    20,     0               }}; //ΑΝ = 1+1+6=8 / ΕΝ = 8
                        
//Σχήμα πρόπλυσης 4 ενεργό / 12 ανενεργό
//                      Α/Α, χρόνος αναμονής σε ms,       λειτουργία
motor_instr instr2[] = {{ 0,                  1000,     toggle_FWD_REV  },
                        { 1,                  4000,     start_turn_slow },
                        { 2,                  1000,     stop_turn       },
                        { 3,                 10000,     stop_motor      },
                        {99,                     0,     0               }};

//Σχήμα στυψίματος1
//                      Α/Α, χρόνος αναμονής σε ms,       λειτουργία
motor_instr instr3[] = {{ 0,                  1000,     spin_FWD        }, //Φορά περιστροφής (ΑΝ)
                        { 1,                  3000,     start_turn_bal  }, //Περιστροφή για 3 sec
                        { 2,                  1000,     stop_turn       }, //
                        { 3,                  2000,     stop_motor      },
                        //------------------------------------------------------------------------
                        { 4,                  1000,     spin_REV        }, //Φορά περιστροφής (ΑΝ)
                        { 5,                  5000,     spin_test       },
                        { 6,                  1000,     stop_turn       },
                        { 7,                  1000,     stop_motor      },
                        //------------------------------------------------------------------------
                        { 8,                  1000,     spin_FWD        },
                        { 9,                  3000,     start_turn_bal  }, //Περιστροφή για 3 sec
                        {10,                  1000,     stop_turn       },
                        {11,                  2000,     stop_motor      },
                        //------------------------------------------------------------------------
                        {12,                  1000,     spin_REV        },
                        {13,                 30000,     start_spin1     },
                        {14,                  1000,     stop_turn       },
                        {15,                  2000,     stop_motor      },
                        {99,                    20,     0               }};

//Εκτελεί το σχήμα πλήσης t για μία φορά (Λειτουργία Non Blocking)
bool progressive_spin = false, wait_flag = false, block_next = false;
unsigned long step_delay = 0;
int step_rpms; 
void motor_turn(motor_instr *t)
     {
      static byte i = 0, state = 0; 
      static unsigned long w_time;
      static bool one_time = false;
      //Αν μόλις ξεκίνησε με την 1η κατάσταση
      if (state == 0 && !one_time)
         {
          w_time = millis(); //Κράτα τον χρόνο
          one_time = true; //Μην ξαναμπείς εδώ
         }
      //Αν έχει περάσει ο χρόνος delay_time τότε
      if ((millis() - w_time >= t[i].delay_time) && !block_next)
         {
          i++; //πάμε στην επόμενη γραμμή
          w_time = millis(); //Μέτρα νέο delay
         }
      //Αλλιώς
      else
         {
          //Αν είμαι στην σωστή γραμμή ή στη τελευταία
          //Εκτελείται μια φορά
          if ((state == t[i].state) || (t[i].state == 99))
             {
              FPtr = t[i].s_func; //Πάρε τον δείκτη στην διαδικασία
              state++; //Δείχνει στην επόμενη κατάσταση
              //Αν είναι η τελευταία κατάσταση
              if (t[i].state == 99)
                 {
                  i = 0; state = 0; //Μηδένισε δείκτες καταστάσεων για την επόμενη λειτουργία
                  CYCLE_OK = true; //Καθολική μεταβλητή χρήσιμη στο cool_turn και warm_turn για να ξέρει πότε τελείωσε ένας κύκλος
                  one_time = false; //Στο επόμενο να ξεκινήσει πάλι από την αρχή
                 }
              //Αλλιώς 
              else
                 FPtr(); //Εκτελεί την εντολή μία φορά
             }
          //Εκτελείται συνέχεια
          if (progressive_spin)
             {
              if (async_timer(step_delay, 200)) //Να πάει στο επόμενο μετά από 1 λεπτό
                 {
                  if (step_rpms >= 8000)
                      step_rpms = 8000;
                  else
                      step_rpms += 200;
                  target_rpm = step_rpms;    
                 }
             }
          else if (wait_flag)
             {
              if (avg_rpm > 500)
                  block_next = true;
              else
                 {
                  block_next = false;
                  wait_flag = false;
                 }
             }
         }
     }

//Αλλάζει την φορά περιστροφής Δεξιά - Αριστερά
void toggle_FWD_REV()
     {
      static bool dir = false;
      if (dir)
         {
          //Serial.println("FWD"); //Debug
          motorFWD(); //Γυρίζει τα τυλίγματα μεταξύ ρότορα και στάτορα
          lcd.img(motor_left, 91, 48, true); //Εικονίδιο στη γραμμή κατάστασης
          dir = false;
         }
      else
         {
          //Serial.println("REV"); //Debug
          motorREV(); //Γυρίζει τα τυλίγματα μεταξύ ρότορα και στάτορα
          lcd.img(motor_right, 91, 48, true); //Εικονίδιο στη γραμμή κατάστασης
          dir = true;
         }
      delay(20);
      avg_rpm = 0; //Μηδενίζει μετρητή RPM 
     }

void spin_FWD()
     {
      motorFWD();
      lcd.img(motor_spin, 91, 48, true);
      delay(20);
      avg_rpm = 0;  
     }

void spin_REV()
     {
      motorREV();
      lcd.img(motor_spin, 91, 48, true);
      delay(20);
      avg_rpm = 0;  
     }

//Αργό γύρισμα με 60 RPMs
void start_turn_slow()
     {
      target_rpm = 1200; //1200 : 20 = 60
      start_flag = true;
     }

//Σταματάει την περιστροφή του τυμπάνου αλλά αφήνει την τάση στο στάτορα
void stop_turn()
     {
      target_rpm = 0;
      start_flag = true;
      progressive_spin = false;
     }

//Ξεκίνημα στυψίματος με ζύγισμα
void start_turn_bal()
     {
      target_rpm = 3600;
      start_flag = true;
     }

void start_spin1()
     {
      target_rpm = 6000;
      start_flag = true;
     }

void spin_test()
     {
      step_rpms = 3000;
      start_flag = true;
      target_rpm = step_rpms;
      progressive_spin = true;
     }
     
//Κόβει την παροχή ρεύματος του στάτορα
void stop_motor()
     {
      motorSTOP();
      lcd.img(motor_idle, 91, 48); //Εικονίδιο στη γραμμή κατάστασης
      wait_flag = true;
     }
