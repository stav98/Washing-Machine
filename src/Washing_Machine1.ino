#include <WiFi.h>

#include <FS.h>
#include "SPIFFS.h" // ESP32 only

#include "definitions.h"
#include "screen.h"
#include "motor.h"
#include "adc.h"
#include "functions.h"
#include "network.h"
//#include "sound.h"
#include "buttons.h"
#include "cmd.h"
#include "commands.h"

unsigned long time_now;

//Λειτουργεί στον πυρήνα 1
void setup() 
     { 
      //Δημιουργία Task στον πυρήνα 0
      xTaskCreatePinnedToCore
           (
            Task1code, /* Συνάρτηση εξυπηρέτησης για το Task */
            "Task1",   /* Όνομα του Task */
            10000,     /* Μέγεθος του stack σε words */
            NULL,      /* Task input parameter */
            0,         /* Προταιρεότητα του TASK */
            NULL,      /* Task handle. */
            0          /* Πυρήνας που θα τρέχει */
           ); 
      cmdInit(115200);
      if (!SPIFFS.begin()) 
         {
          Serial.println("SPIFFS initialisation failed!");
          while (1) yield(); // Stay here twiddling thumbs waiting
         }
      Serial.println("\r\nSPIFFS available!");
      
      lcd.begin(DISP_DATA, DISP_CLK);
      initADC();
      //  initSOUND();
      //initBUTTONS();
      initWebServer();
      Serial.println("Starting ...");
      //  sound_demo();
      //front_page_stat();
      //intro();

      pinMode(VALVE1, OUTPUT);
      pinMode(VALVE2, OUTPUT);
      pinMode(PUMP, OUTPUT);
      pinMode(HEATER, OUTPUT);
      pinMode(DOOR_LOCK, OUTPUT);
      pinMode(WATER_L1, INPUT_PULLUP);
      //Ταχογεννήτρια. Αυτό λειτουργεί στον πυρήνα 0
      //pinMode(TACHO_PIN, INPUT_PULLUP);
      //attachInterrupt(TACHO_PIN, tacho_ISR, FALLING);
      
      init_motor();
      pinMode(TEST_PIN, OUTPUT);
      
      time_now = millis();
      door_lock();
      dest_w_temp = 31;
     }

//Τρέχει στον πυρήνα 0
void Task1code( void * pvParameters )
     {
      //------ Δεύτερο setup γι αυτόν τον πυρήνα ------
      //Ταχογεννήτρια.
      pinMode(TACHO_PIN, INPUT_PULLUP);
      attachInterrupt(TACHO_PIN, tacho_ISR, FALLING);
      //Εντολές τερματικού
      add_commands();
      cmd_display(); //Εμφάνισε prompt στο τερματικό
      //Πλήκτρα
      initBUTTONS();
      //------ Δεύτερο loop γι αυτόν τον πυρήνα -------
      for(;;)
         {
          //Serial.print("this running on core ");
          //Serial.println(xPortGetCoreID());
          //Αποκωδικοποίηση εντολών τερματικού.
          cmdPoll();
          //Έλεγχος πλήκτρων.
          butn_Up.CheckBP(); //Έλεγχος της κλάσης για το πάτημα του button
          butn_Down.CheckBP();
          butn_Ok.CheckBP();
          delay(1);
         } 
     }

//int rr = 1000;
bool once_flag = false;
//Λειτουργεί στον πυρήνα 1
void loop() 
     {
      //Έλεγχος πλήκτρων. Αυτό λειτουργεί στον πυρήνα 0
      //butn_Up.CheckBP(); //Έλεγχος της κλάσης για το πάτημα του button
      //butn_Down.CheckBP();
      //butn_Ok.CheckBP();
      if (!once_flag)
         {
          lcd.clear();
          front_page();
          status_bar();
          once_flag = true;
         }
      //Κάθε 1 sec
      if ((millis() - time_now) >= 1000)
         {
          time_now = millis();
          read_Wtemp();
          lcd.gotoxy(108, 24);
          lcd.printf("%5d", avg_rpm); 
          lcd.gotoxy(38, 56);
          lcd.print(w_temp);
          //if (rr >= 8000)
          //    rr = 8000;
          //else
          //    rr += 100;
          //target_rpm = rr;
         }

      //=== Εκτέλεση προγράμματος συνέχεια ===
      //Αν η πόρτα είναι κλειστή τότε
      if (chk_door())
         {
          washing1(scheme1); //Εκτέλεσε πρόγραμμα
         }
      //Αλλιώς
      else
         {
          valve(0);  //Σταμάτα εισαγωγή νερού
          valve_delay = 0;
          state = 0;
          r_cnt1 = 0;
          AUX1 = false;
         }
      //======================================
 
      //lcd.clear();
      //lcd.selectFont(font_fixed_5x8);
      //lcd.selectFont(font_fixed_3x5);
      //lcd.selectFont(font_cp437);
      //lcd.textSize(1);
  
      //lcd.print(PGMSTR(t));
      // black box  
      //lcd.clear (6, 40, 30, 63, 0xFF);
      // draw text in inverse
      //lcd.gotoxy (40, 40);
      //lcd.setInv(true);
      //lcd.print ("Stavros SV6GMP");
      //lcd.setInv(false);
      // bit blit in a picture
      //lcd.gotoxy (40, 56);
      //lcd.blit (picture, sizeof picture);
  
      // draw a framed rectangle
      //lcd.frameRect (40, 49, 60, 53, 1, 1);

      // draw a white diagonal line
      //lcd.line (6, 40, 30, 63, 0);
      //lcd.line (0, 40, 127, 40, 1);
      //lcd.selectFont(font_fixed_5x8);
      //lcd.textSize(1);
      //delay(2000);
      //lcd.clear();
      //lcd.print("Circle drawing:");
      //lcd.circle(20,32,16,1);
      //lcd.fillCircle(60,32,16,1);
      //lcd.gotoxy(80,56);
      //delay(2000);
      //lcd.clear();
      //lcd.selectFont(font_fixed_5x8);
      //lcd.textSize(1);
      //lcd.setInv(0);
      //lcd.print(PGMSTR(t)); //Ίδιο με το F("....")
      //delay(2000);
      //lcd.clear();
      //lcd.img(pic1, 20, 20, 1);
      //delay(2000);
      delay(1);
     }
