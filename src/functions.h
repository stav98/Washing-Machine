//Τα χρονικά σημεία μεταξύ των ενεργειών
unsigned short points[30]; //Χρόνοι σε sec

//Σχεδιάζει κάθετες χαραγές στους προκαθορισμένους χρόνους
void v_lines(int p, int maxp)
     {
      byte x;
      x = map(p, 0, maxp, 4, 123); //Για x από 4 έως 123 px
      lcd.line(x, 8, x, 13, 1);  //Κάθετη γραμμή
     }

//Παρουσίαση της γραμμής προόδου
void progress_bar(int p, int maxp)
     {
      byte x, i;
      x = map(p, 0, maxp, 4, 123);
      lcd.fillRect (0, 8, 127, 14, 0); //Σβήνει παλιά
      lcd.frameRect (0, 10, 127, 12, 1, 1); //Πλαίσιο
      lcd.fillCircle(x, 11, 4, 1); //Κύκλος που κινείται
      lcd.line(0, 11, x, 11, 1); //Γέμισμα πλαισίου αριστερά του κύκλου
      i = 0; //Ξεκίνα από 1η τιμή
      //Ζωγραφίζει τις χαραγές με κάθε ανανέωση της μπάρας
      while (points[i] != 999) //Μέχρι το τέλος του πίνακα
             v_lines(points[i++], maxp); //Κάθετες χαραγές
     }

//Εμφανίζει τον υπολοιπόμενο χρόνο
void print_rem_time(int t)
     {
      lcd.selectFont(font_fixed_5x8);
      lcd.gotoxy(96, 16);
      lcd.printf("%03d'", t);
      lcd.selectFont(font_fixed_3x5);
     }

//Εμφανίζει την λειτουργία που εκτελείται την συγκεκριμένη στιγμή
void print_msg(const char* m)
    {
     lcd.selectFont(font_fixed_5x8);
     lcd.gotoxy (0, 32);
     lcd.print("                     ");
     lcd.gotoxy (0, 32);
     lcd.print(PGMSTR(m)); 
    }
     
//Κάθε 1sec ελέγχει αν έχει γίνει interrupt zero cross δηλαδή η πόρτα είναι κλειστή
bool chk_door()
     {
      static unsigned long door_intrv = 0;
      static bool v = false, v_pre = false;
      if (millis() - door_intrv > 1000)
         {
          if (en_flag)
              v = true;
          else
              v = false;
          if (v != v_pre)
             {
              if (v)
                 lcd.img(door_ok, 2, 48, true);
              else
                 lcd.img(door_nok, 2, 48, false);
             }
          v_pre = v;
          door_intrv = millis();
          en_flag = false;
         }
      return v;
     }

//Σε διάστημα 1 sec ελέγχει την στάθμη νερού και αν είναι σωστή
//επιστρέφει true και αλλάζει το σύμβολο στο status bar 
bool chk_waterL1()
     {
      static unsigned long press_intrv = 0;
      static bool v = false, v_pre = false;
      if (millis() - press_intrv > 1000)
         {
          if (digitalRead(WATER_L1) == true)
              v = true;
          else
              v = false;
          if (v != v_pre)
             {
              if (v)
                 lcd.img(pressure_sw_ok, 19, 48, true);
              else
                 lcd.img(pressure_sw_nok, 19, 48, false);
             }
          v_pre = v;
          press_intrv = millis();
         }
      return v;
     }

//Ανοίγει τις βαλβίδες για εισαγωγή νερού
void valve(byte n)
     {
      switch (n)
             {
              case 0:
                 waterinp_none();
                 lcd.img(valve_idle, 57, 48);
                 break;
              case 1:
                 waterinp1();
                 lcd.img(valve1, 57, 48, true);
                 break;
              case 2:
                 waterinp2();
                 lcd.img(valve2, 57, 48, true);
                 break;
              case 3:
                 waterinp3();
                 lcd.img(valve3, 57, 48, true);
                 break;
              default:
                 waterinp_none();
                 lcd.img(valve_idle, 57, 48);
             }
     }

//Ανάβει ή σβήνει την αντίσταση
void set_heater(bool state)
     {
      if (state)
         {
          heat_on();
          lcd.img(heater, 108, 48, true);
         }
      else
         {
          heat_off();
          lcd.img(heater, 108, 48);
         }
     }

//Ανάβει ή σβήνει την αντλία
void set_pump(bool state)
     {
      if (state)
         {
          pump_on();
          lcd.img(pump, 74, 48, true);
         }
      else
         {
          pump_off();
          lcd.img(pump, 74, 48);
         }
     }
     
//Εισαγωγή νερού από θέση 1, 2 ή 3 (3=1+2)
unsigned long valve_delay = 0;
bool water(byte inp)
     {
      bool val = false;
      //Αν η βαλβίδα είναι κλειστή και δεν έχει συμπληρώσει νερό
      if (!AUX1 && !chk_waterL1())
         {
          if (async_timer(valve_delay, 3000)) //Να ανοίξει μετά από 3sec
             {
              valve(inp);
              AUX1 = true;
             }
         }
      //Αλλιώς αν είναι ανοιχτή και συμπλήρωσε νερό 
      else if (AUX1 && chk_waterL1())
         {
          if (async_timer(valve_delay, 3000)) //Να κλείσει μετά από 3sec
             {
              valve(0);
              val = true;    //Επιστροφή OK
              AUX1 = false;
             }
         }
      //Αλλιώς αν υπάρχει νερό μέσα τότε να πάει στο επόμενο βήμα μετά από 2sec
      else if (chk_waterL1())
         {
          if (async_timer(valve_delay, 2000))
              val = true;   //Επιστροφή OK
         }
      //Για όσο βάζει νερό   
      else
          valve_delay = 0; //Μηδένισε timer
      return val;
     }

//Κάνει ν περιστροφές αργά (1200rpm - 60rpm) αριστερά - δεξιά χωρίς να ζεσταίνει το νερό.
byte r_cnt1 = 0;
void cool_turn(byte times)
     {
      if (CYCLE_OK) //Αν τελείωσε ο κύκλος (αλλάζει στο motor.h/stop_stator())
         {
          //Serial.println(r_cnt1); //Debug
          if (r_cnt1 < (times - 1)) //Αν δεν έφτασε τις φορές - 1
             {
              r_cnt1++; //Αύξησε μετρητή
              CYCLE_OK = false; //Πάμε για τον επόμενο κύκλο
              }
          else //Έγινε ο αριθμός περιστροφών
              {
               state++;  //μπορεί να πάει στην επόμενη εργασία 
               CYCLE_OK = false; //Προετοιμασία για την επόμενη εργασία
               r_cnt1 = 0;
              }
         }
      else //Δεν τελείωσε ο κύκλος
         {
          motor_control(); //PID control
          motor_turn(instr1); //Εκτελεί ένα κύκλο αργής πλύσης
         }
     }
     
//Κάνει περιστροφές αργά μέχρι να συμπληρωθεί η ζητούμενη θερμοκρασία
void warm_turn()
     {
      static bool aux = false;
      //Άναψε μια φορά την αντίσταση 
      if (!aux)
         {
          set_heater(true);
          aux = true; //Μην ξαναμπείς εδώ
         }
      if (CYCLE_OK)
         {
          //Serial.println(r_cnt1); //Debug
          r_cnt1++;
          if (w_temp < dest_w_temp) 
              CYCLE_OK = false;
          else
              {
               set_heater(false);
               state++;
               CYCLE_OK = false;
               aux = false;
               r_cnt1 = 0;
              }
         }
      else
         {
          motor_control();
          motor_turn(instr1);
         }
     }

//Συμπληρώνει λίγο κρύο νερό για λίγο χρόνο 
void extra_water(byte inp, byte secs)
     {
      static bool aux = false;
      if (!aux)
         {
          valve_delay = 0; //Μηδένισε timer
          aux = true;
         }
      //Αν η βαλβίδα είναι κλειστή
      if (!AUX1)
         {
          if (async_timer(valve_delay, 3000)) //Να ανοίξει μετά από 3sec
             {
              valve(inp);
              AUX1 = true;
              valve_delay = 0; //Μηδένισε timer
             }
         }
      //Αλλιώς αν είναι ανοιχτή και συμπλήρωσε νερό 
      else
         {
          if (async_timer(valve_delay, (secs * 1000))) //Να κλείσει μετά από 10sec
             {
              valve(0);
              AUX1 = false;
              aux = false;
              valve_delay = 0; //Μηδένισε timer
              state++;
             }
         }
     }

//Αντλεί το νερό
unsigned long pump_delay = 0;
void pump_water()
     {
      static bool aux = false;
      if (!aux)
         {
          if (async_timer(pump_delay, 3000)) //Να ανοίξει μετά από 3sec
             {
              set_pump(true);
              aux = true;
             }
         }
      else
         {
          if (async_timer(pump_delay, 60000)) //Να πάει στο επόμενο μετά από 1 λεπτό
             {
              state++;
             }
         }
     }

//Στύψιμο ρούχων
void spin_dram()
     {
      if (CYCLE_OK)
         { 
          CYCLE_OK = false;
          state++;
         }
      else
         {
          motor_control();  //Εκτελείται συνέχεια
          motor_turn(instr3); //Συνέχεια
         }
     }

//typedef void (*FuncPtr)(void);  //Ορίζεται στο motor.h

typedef struct _washing_scheme
        {
         byte state;
         byte op_code;
         byte param1;
         byte param2;
         int progress;
        } washing_scheme;

FuncPtr FPtr1;

typedef struct _functions
        {
         FuncPtr s_func;
         char debug_msg[15];
         const char *label;
        } functions;


void Eisagwgi(void);
void coolTurn_water(void);
void warmTurn(void);
void extraWater(void);
void coolTurn(void);
void pumpWater(void);
void telos(void);

functions leitourgies[] = {{ Eisagwgi         , "Eisodos 1"      ,lbl_eisag1      },  //0 - Εισαγωγή
                           { coolTurn_water   , "Kryo plysimo"   ,lbl_plisi_kr    },  //1 - Κρύο πλύσιμο με εισαγωγή
                           { warmTurn         , "Zesto plysimo"  ,lbl_plisi_zes   },  //2 - Ζεστό πλύσιμο
                           { extraWater       , "Extra Nero"     ,lbl_eisag_sympl },  //3 - Συμπλήρωση κρύου νερού
                           { coolTurn         , "Plysimo"        ,lbl_plisi       },  //4 - Πλύσιμο χωρίς ζέσταμα
                           { pumpWater        , "Ejagwgh"        ,lbl_antlisi     },  //5 - Απάντληση
                           { spin_dram        , "Stypsimo"       ,lbl_spin        },  //6 - Στύψιμο
                           { telos            , "Telos"          ,lbl_finish      }}; //7 - Τέλος

//Σχήμα πλύσης
//                            Α/Α,        Κωδικός  ,  Παράμετρος 1, Παράμετρος 2,     Χρόνος sec που 
//                         κατάσταση ,  λειτουργίας,              ,             ,  διαρκεί η λειτουργία
/*washing_scheme scheme1[] = {{  0     ,            0,             1,            0,          20      }, //[Εισαγωγή] 20-40sec ανάλογα με τη πίεση
                            {  1     ,            1,             1,            5,          80      }, //[Κρύο πλύσιμο - εισαγ.] n * 16sec φορές περιστροφής
                            {  2     ,            2,             0,            0,          96      }, //[Ζεστό πλύσιμο] ανάλογα με την θερμοκρασία
                            {  3     ,            3,             2,           20,          23      }, //[Συμπήρωση νερού]
                            {  4     ,            4,             6,            0,          96      }, //[Κρύο πύσιμο] n * 16sec
                            {  5     ,            5,             0,            0,          60      }, //[Άντληση] 1 λεπτό
                            {  6     ,            6,             0,            0,          50      }, //[Στύψιμο]
                            { 99     ,            7,             0,            0,           0      }};//[Τέλος]
*/

//Σχήμα πλύσης
//                            Α/Α,        Κωδικός  ,  Παράμετρος 1, Παράμετρος 2,     Χρόνος sec που 
//                         κατάσταση ,  λειτουργίας,              ,             ,  διαρκεί η λειτουργία
washing_scheme scheme1[] = {{  0     ,            0,             1,            0,          20      }, //[Εισαγωγή] 20-40sec ανάλογα με τη πίεση
                            {  1     ,            6,             0,            0,          60      },
                            {  2     ,            1,             1,            5,          80      }, //[Κρύο πλύσιμο - εισαγ.] n * 16sec φορές περιστροφής
                            {  3     ,            2,             0,            0,          96      }, //[Ζεστό πλύσιμο] ανάλογα με την θερμοκρασία
                            {  4     ,            3,             2,           20,          23      }, //[Συμπήρωση νερού]
                            {  5     ,            4,             6,            0,          96      }, //[Κρύο πύσιμο] n * 16sec
                            {  6     ,            5,             0,            0,          60      }, //[Άντληση] 1 λεπτό
                            {  7     ,            6,             0,            0,          50      }, //[Στύψιμο]
                            { 99     ,            7,             0,            0,           0      }};//[Τέλος]

//Διαδικασία πλυσίματος βάσει της παραμέτρου τύπου washing_scheme
unsigned long dif_time, refr_time = 0;
void washing1(washing_scheme *t)
     {
      static byte i = 0;
      static bool one_time = false;
      static int max_time, cur_progress, d;
      byte j;
      int stime;
      //Για όσο το stop_flag είναι false τρέξε το πρόγραμμα
      if (!stop_flag)
         {
          if (state == t[i].state) //Αν η κατάσταση Α/Α είναι ίση με την τρέχουσα
             {
              //Μόνο μια φορά στην αρχή του προγράμματος
              if (state == 0 && !one_time)
                 {
                  //Υπολογισμός του μέγιστου χρόνου και σημείων εμφάνισης χαραγών
                  j = 0; max_time = 0; points[0] = 999;
                  //Όσο δεν έχει φτάσει στην τελευταία λειτουργία
                  while (t[j].state != 99) 
                        {
                         max_time += t[j].progress;
                         points[j++] = max_time; //Αποθήκευσε στον πίνακα το χρονικό όριο κάθε λειτουργίας για σχεδιασμό χαραγής
                        }
                  points[j-1] = 999; //Τελευταίο
                  Serial.println(max_time); //Debug
                  //Serial.println(leitourgies[t[0].op_code].debug_msg); //Debug
                  print_msg(leitourgies[t[0].op_code].label);
                  progress_bar(0, max_time); //Εμφάνισε μπάρα προόδου
                  cur_progress = 0;
                  dif_time = millis(); //Debug
                  one_time = true;
                  print_rem_time(0);
                 }
              //Ανανέωση progress bar κάθε 10 sec
              if (millis() - refr_time > 10000)
                 {
                  refr_time = millis();
                  //Αν δεν έχει ξεπεράσει το μέγιστο όριο χρόνου της λειτουργίας (χαραγή)
                  if (d < t[i].progress) //t[i+1]
                      d += 10; //Αύξησε μικρομετρικά την θέση του progress bar 
                  progress_bar(cur_progress + d, max_time); //Αύξησε το progress bar κατά το ποσοστό
                  print_rem_time((int)(max_time - cur_progress - d) / 60); //Εμφάνισε τον υπολοιπόμενο χρόνο
                 }
              FPtr1 = leitourgies[t[i].op_code].s_func;; //Διάβασε δείκτη συνάρτησης
              param1 = t[i].param1;
              param2 = t[i].param2;
              FPtr1(); //Εκτέλεσε συνάρτηση
             }
          else //Αλλιώς, τελείωσε η προς εκτέλεση λειτουργία και το state πήγε στην επόμενη θέση  
             {
              //Serial.print(leitourgies[t[i+1].op_code].debug_msg); Serial.print(" "); //Debug
              Serial.println(millis() - dif_time); //Debug
              dif_time = millis(); //Debug
              d = 0;
              cur_progress += t[i].progress;
              print_msg(leitourgies[t[i+1].op_code].label);
              progress_bar(cur_progress, max_time); //Αύξησε το progress bar κατά το ποσοστό
              print_rem_time((int)(max_time - cur_progress) / 60); //Εμφάνισε τον υπολοιπόμενο χρόνο
              i++; //Πήγαινε στην επόμενη λειτουργία
             }
          //Αν έφτασε στο τέλος
          if (t[i].state == 99)
             {
              i = 0;
              one_time = false;
              stop_flag = true; //Σταμάτα το πρόγραμμα
             }
         }
     else
         avg_rpm = 0;
     }

void Eisagwgi()
{
 if (water(param1))
     state++;
}

void coolTurn_water()
{
 water(param1);
 cool_turn(param2);
}

void warmTurn()
{
 warm_turn();
}

void extraWater()
{
 extra_water(param1, param2);
}

void coolTurn()
{
 cool_turn(param1);
}

void pumpWater()
{
 chk_waterL1();
 pump_water();
}

void telos()
{
 stop_flag = true;
}
  
