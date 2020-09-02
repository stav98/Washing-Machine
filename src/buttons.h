#define BTN_UP    14
#define BTN_DOWN  0
#define BTN_OK    33
#define BTN_PWR   13
#define PULL_UP     HIGH
void initBUTTONS(void);
void Butn_Up_Click(void);
void Butn_Up_Long_Click(void);
void Butn_Up_Dbl_Click(void);

void Butn_Down_Click(void);

void Butn_Pwr_Long_Click(void);

class Buttons
{
 private:
   int _pin;

    // Properties //
   ////////////////

   // Debounce period to prevent flickering when pressing or releasing the button (in ms)
   int Debounce;
   // Max period between clicks for a double click event (in ms)
   int DblClickDelay;
   // Hold period for a long press event (in ms)
   int LongPressDelay;

    // Variables //
   ///////////////

   // Value read from button
   boolean state;
   // Last value of button state
   boolean _lastState;
   // whether we're waiting for a double click (down)
   boolean _dblClickWaiting;
   // whether to register a double click on next release, or whether to wait and click
   boolean _dblClickOnNextUp;
   // whether it's OK to do a single click
   boolean _singleClickOK;

   // time the button was pressed down
   long _downTime;
   // time the button was released
   long _upTime;

   // whether to ignore the button release because the click+hold was triggered
   boolean _ignoreUP;
   // when held, whether to wait for the up event
   boolean _waitForUP;
   // whether or not the hold event happened already
   boolean _longPressHappened;

 public:
   void (*OnClick)(void);
   void (*OnDblClick)(void);
   void (*OnLongPress)(void);

 Buttons()
     {
      // Initialization of properties
      Debounce = 50; //30
      DblClickDelay = 300; //300
      LongPressDelay = 2000;

      // Initialization of variables
      state = true;
      _lastState = true;
      _dblClickWaiting = false;
      _dblClickOnNextUp = false;
      _singleClickOK = false; //Default = true
      _downTime = -1;
      _upTime = -1;
      _ignoreUP = false;
      _waitForUP = false;
      _longPressHappened = false;
     }

   void Configure(int pin)
      {
       _pin = pin;
       pinMode(_pin, INPUT_PULLUP); //Κάνε το pin είσοδο Pull Up
      }

   void CheckBP(void)
      {
       int resultEvent = 0;
       long millisRes = millis();
       state = digitalRead(_pin);
       //---- Κουμπί πατήθηκε ----
       if (state != PULL_UP && _lastState == PULL_UP  && (millisRes - _upTime) > Debounce)
          {
           //Serial.println("button down"); //Debug
           _downTime = millisRes;
           _ignoreUP = false;
           _waitForUP = false;
           _singleClickOK = true;
           _longPressHappened = false;
           if ((millisRes - _upTime) < DblClickDelay && _dblClickOnNextUp == false && _dblClickWaiting == true)
               _dblClickOnNextUp = true;
           else
               _dblClickOnNextUp = false;
           _dblClickWaiting = false;
          }
       // Button released
       else if (state == PULL_UP && _lastState != PULL_UP && (millisRes - _downTime) > Debounce)
          {
           //Serial.println("button up");
           if (_ignoreUP == false) //Replace "(!_ignoreUP)" by "(not _ignoreUP)"
              {
               _upTime = millisRes;
               if (_dblClickOnNextUp == false)
                   _dblClickWaiting = true;
               else
                  {
                   resultEvent = 2;
                   _dblClickOnNextUp = false;
                   _dblClickWaiting = false;
                   _singleClickOK = false;
                  }
              }
          }

       // Test for normal click event: DblClickDelay expired
       if (state == PULL_UP && (millisRes - _upTime) >= DblClickDelay && _dblClickWaiting == true && _dblClickOnNextUp == false && _singleClickOK == true && resultEvent != 2)
          {
           resultEvent = 1;
           _dblClickWaiting = false;
          }

       //----- Έλεγχος για παρατεταμένο πάτημα -----
       //Είναι πατημένο και ο χρόνος πέρασε την καθυστέρηση του LongPressDelay
       if (state != PULL_UP && (millisRes - _downTime) >= LongPressDelay)
          {
           //Πυροδότησε το παρατεταμένο πάτημα
           // Αν δεν πατήθηκε πριν παρατεταμένα
           if (_longPressHappened == false)
              {
               resultEvent = 3;
               _waitForUP = true;
               _ignoreUP = true;
               _dblClickOnNextUp = false;
               _dblClickWaiting = false;
               _downTime = millis(); //ισως δεν χρειάζεται
               _longPressHappened = true;
              }
          }
       _lastState = state;
       //if (resultEvent!=0)
       //  Serial.println((String)"resultEvent: " + (String) resultEvent);

       if (resultEvent == 1 && OnClick) OnClick();
       if (resultEvent == 2 && OnDblClick) OnDblClick();
       if (resultEvent == 3 && OnLongPress) OnLongPress();
       //  if (resultEvent != 0)
       //    Usb.println(resultEvent);
      } //Τέλος της CheckBP()
};  //Τέλος ορισμού της κλάσης

Buttons butn_Up, butn_Down, butn_Pwr;

void initBUTTONS()
{
 //pinMode(BTN_UP, INPUT_PULLUP);
  //pinMode(BTN_DOWN, INPUT_PULLUP);
  //pinMode(BTN_OK, INPUT_PULLUP);
  //pinMode(BTN_PWR, INPUT_PULLUP);
 butn_Up.Configure(BTN_UP);
 butn_Up.OnClick = Butn_Up_Click;
 butn_Up.OnLongPress = Butn_Up_Long_Click;
 butn_Up.OnDblClick = Butn_Up_Dbl_Click;

 butn_Down.Configure(BTN_DOWN);
 butn_Down.OnClick = Butn_Down_Click;

 butn_Pwr.Configure(BTN_PWR);
 butn_Pwr.OnLongPress = Butn_Pwr_Long_Click;
}

//Όταν πατιέται ένα κλικ στο button
void Butn_Up_Click()
{
 Serial.println("Button Up Click"); //Debug
 //playTone(600, 30);
}

//Όταν πατιέται παρατεταμένα
void Butn_Up_Long_Click()
{
 Serial.println("Button Up Long Click"); //Debug
 //playTone(2500, 300);
}

//Όταν πατιέται διπλό κλικ
void Butn_Up_Dbl_Click()
{
 Serial.println("Button Up Double Click"); //Debug 
 //playTone(2000, 100);
}

void Butn_Down_Click()
{
 //playTone(600, 30);
}

void Butn_Pwr_Long_Click()
{
// playTone(2500, 300);
}
