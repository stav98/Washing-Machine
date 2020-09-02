#define BUZZER_PIN    25
#define REF_CLK       5000 //Θα κάνει διακοπή με συχνότητα 5KHz
//volatile unsigned long ph_acc, divider; //32bit
volatile unsigned short ph_acc, divider; //16bit - Συσωρευτής και διαιρέτης - βήμα 5000 / 65536 = 0,0763Hz
volatile unsigned long int_count, tim_duration; //Το unsigned int είναι το ίδιο με το unsigned long δηλ. 32bit
volatile byte sqr_wave;
volatile boolean play_tone;
unsigned long beep_start;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
 
//Δοκιμασμένο μέχρι 100KHz
//FOUT = (divider * REF_CLK) / (2 ^ 32)
//Βήμα = REF_CLK / (2 ^ 32)
//Μέγιστη συχνότητα = REF_CLK / 2
void IRAM_ATTR onTimer() 
 {
  portENTER_CRITICAL_ISR(&timerMux);
  ph_acc += divider; // Πρόσθεσε στον συσωρευτή τον divider
  //sqr_wave = ph_acc >> 31; //32bit
  sqr_wave = ph_acc >> 15; //16bit Εφόσον δεν υπάρχει πίνακας ημιτόνου, πάρε το τελευταίο bit για τετράγωνο
  if (play_tone) int_count++;
  portEXIT_CRITICAL_ISR(&timerMux);
  if (play_tone && (int_count < tim_duration))
      digitalWrite(BUZZER_PIN, sqr_wave);  
  else
      play_tone = false;
 }

 void initSOUND()
  {
   play_tone = false;
   pinMode(BUZZER_PIN, OUTPUT);
   timer = timerBegin(0, 80, true); //80MHz : 80 = 1MHz  prescaler
   timerAttachInterrupt(timer, &onTimer, true);
   timerAlarmWrite(timer, 200, true); //1MHz : 200 = 5000Hz
   //divider = pow(2, 32) * sfreq / REF_CLK; //32bit
   divider = 20000; //16bit
   timerAlarmEnable(timer);
  }

 void playTone(unsigned short freq, unsigned short duration)
  {
   //divider = pow(2, 32) * freq / REF_CLK; //32bit
   divider = pow(2, 16) * freq / REF_CLK; //16bit
   portENTER_CRITICAL(&timerMux);
   int_count = 0;
   tim_duration = duration * 10;
   play_tone = true;
   portEXIT_CRITICAL(&timerMux);
  }

 void sound_demo()
  {
   unsigned short i;
   for(i = 100; i <= 2400; i += 100)
      {
       playTone(i, 50);
       delay(60);
      }
  }
