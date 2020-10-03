#define W_TEMP_PIN     36

#define THERMISTORNOMINAL 4829      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3980
// the value of the 'other' resistor
#define SERIESRESISTOR 4700  

int samples[NUMSAMPLES];


void initADC()
     {
      //Γραμμικό 0.12 - 3.0V (10 - 3720)
      //analogSetWidth(9); //9 ή 12
      //analogSetPinAttenuation(15, ADC_6db); //ADC_0db : 1V, ADC_2_5db : 1.34V, ADC_6db : 1.5V, ADC_11db : 3.6V (Default)
     }

/*void readInstruments()
     {
      unsigned short adc_volts, adc_amperes, adc_fwd, adc_rev;
      adc_volts = analogRead(ADC_VOLTS);
      adc_amperes = analogRead(ADC_AMPS);
      adc_fwd = analogRead(ADC_FWD);
      adc_rev = analogRead(ADC_REV);
      volts = map(adc_volts, 0, 4096, 0, 230);
      amperes = map(adc_amperes, 0, 4096, 0, 60);
      fwd = map(adc_fwd, 0, 4096, 0, 25000);
      rev = map(adc_rev, 0, 4096, 0, 1000);
      //psu_temp = i2c_getTemp(PSU_TEMP);
     }*/

void read_Wtemp()
     {
      byte i;
      float average;
      // take N samples in a row, with a slight delay
      for (i=0; i< NUMSAMPLES; i++) 
          {
           samples[i] = analogRead(W_TEMP_PIN);
           delay(10);
          }
      average = 0;
      for (i=0; i< NUMSAMPLES; i++)
           average += samples[i];
      average /= NUMSAMPLES;
 
      //Serial.print("Average analog reading "); //Debug
      //Serial.println(average); //Debug
      //Διαιρέτης τάσης 4Κ7 στα 3V3 και NTC EPCOS K276 με 4Κ8 στη γη. Vadc = (Vmax * Rx)/(4700 + Rx)
      //Μετατροπή σε αντίσταση
      //Για τάση 1.51V διαβάζω τιμή ADC 1664. Με απλή μέθοδο των τριών υπολογίζω την τιμή που κανονικά έπρεπε να διαβάσω
      // 3.29V    4095
      // 1.51V     X;      x = (4095 * 1.51) / 3.29 = 1879 έπρεπε να διαβάζω
      // 1879 / 1664 = 1.13 συντελεστής διόρθωσης ADC
      average *= 1.13;
      average = 4095 / average - 1;
      average = SERIESRESISTOR / average;
      //Η αντίσταση είναι 3990 και την επαληθεύω με ωμόμετρο. Η θερμοκρασία νερού 29.3 βαθμούς
      //Serial.print("Thermistor resistance "); //Debug
      //Serial.println(average); //Debug
      //Υπολογισμός θερμοκρασίας από αντίσταση
      float steinhart;
      steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
      steinhart = log(steinhart);                  // ln(R/Ro)
      steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
      steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
      steinhart = 1.0 / steinhart;                 // Invert
      steinhart -= 273.15;                         // convert to C
      //Serial.print("Temperature "); //Debug
      //Serial.print(steinhart); //Debug
      //Serial.println(" *C"); //Debug
      w_temp = round(steinhart);
      //Serial.println(w_temp); //Debug
     }
     
