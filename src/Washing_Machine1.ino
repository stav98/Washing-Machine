#include <WiFi.h>

#include <FS.h>
#include "SPIFFS.h" // ESP32 only

#include "definitions.h"
#include "screen.h"
#include "motor.h"
#include "adc.h"
#include "network.h"
//#include "sound.h"
#include "buttons.h"
#include "cmd.h"
#include "commands.h"

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
      initBUTTONS();
      initWebServer();
      Serial.println("Starting ...");
      //  sound_demo();
      //front_page_stat();
      //intro();
     
      
      init_motor();
      pinMode(TEST_PIN, OUTPUT);
      
      time_now = millis();

      //add_commands();
      //cmd_display(); //Εμφάνισε prompt στο τερματικό
      Serial.println(sizeof(int));
      Serial.println(sizeof(long int));
      Serial.println(sizeof(float));
      Serial.println(sizeof(double));
      Serial.println(sizeof(short));
     }

//Τρέχει στον πυρήνα 0
void Task1code( void * pvParameters )
     {
      //------ Δεύτερο setup γι αυτόν τον πυρήνα ------
      pinMode(TACHO_PIN, INPUT_PULLUP);
      attachInterrupt(TACHO_PIN, tacho_ISR, FALLING);
      add_commands();
      cmd_display(); //Εμφάνισε prompt στο τερματικό
      //------ Δεύτερο loop γι αυτόν τον πυρήνα -------
      for(;;)
         {
          //Serial.print("this running on core ");
          //Serial.println(xPortGetCoreID());
          cmdPoll();
          delay(50);
         } 
     }

//Λειτουργεί στον πυρήνα 1
void loop() 
     {
      //butn_Up.CheckBP(); //Έλεγχος της κλάσης για το πάτημα του button
      //butn_Down.CheckBP();
      //butn_Pwr.CheckBP();
      //if (digitalRead(17) == 0)
      //readInstruments();
      //motor_speed = 180; //0 - 430  Το μοτέρ αρχίζει να γυρίζει με 60 και με 390 φτάνει 25000 RPM χωρίς φορτίο
      //dim = 490 - motor_speed; //Full=60, Start=490
      //cmdPoll();
      if (!once_flag)
         {
          lcd.clear();
          front_page();
          status_bar();
          once_flag = true;
         }
      if ((millis() - time_now) >= 500)
         {
          time_now = millis();
          //lcd.clear();
          //front_page();
          //status_bar();
          read_Wtemp();
          lcd.gotoxy(82, 32);
          lcd.printf("%5d", avg_rpm); 
          //lcd.print(avg_rpm);
          lcd.gotoxy(38, 56);
          lcd.print(w_temp);
         //- if (motor_speed >= 190)
         //-     motor_speed = 20;
          //else
              //motor_speed += 10;
          //dim = 1000 - motor_speed;
         }
      motor_control();
      plisi_turn();
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
         
      
      //delay(1);
      
     }
