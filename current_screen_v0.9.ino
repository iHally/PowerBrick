#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "math.h"
#include <avr/sleep.h>
#include <avr/wdt.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
 int screenGroundPin = 9;

// watchdog interrupt
ISR (WDT_vect) {
  wdt_disable();  // disable watchdog
}  // end of WDT_vect

    float Voltage = 0;
    //float b2V = 0;
    float Current = 0;
    float Charge = 100;
    float Power = 0;
    float inputVoltage = 0;
    float Battery1 = 0;
    float Battery2 = 0;
    bool firstWakeup = true;
    bool charging = false;
    bool mosfet = true;
    bool screen = false;
    unsigned long idleTime = 0;
    unsigned long upTime = 0;
    unsigned long wakeTime = 0;
    unsigned long lastTime = 0;
    unsigned long loopTime = 0;
    //delete these after testing
    //unsigned long millisNow = 0;
    //unsigned long usTime = 0;
    //unsigned long usLast = 0;
    //int us = 0;
    float Valpha = 0.3;
    float Calpha = 0.3;
    float Palpha = 0.002;
    int i = 0;
    int error = 0;
    String errorString = "";
    


void setup() {
  
  /*
  Serial.begin(115200);
  Serial.println("Basic Weather Station");
  */

    pinMode(screenGroundPin, OUTPUT);
    turnDisplayOn(100);
    digitalWrite(screenGroundPin, LOW); //ground pin for screen. Low-on High-off
    //delay(1000);
        wakeTime=millis();
        lastTime=wakeTime-1;
}
void loop(){
    
    measureVoltages();
    measureCurrent();
    
    //get the time
    unsigned long millisNow = millis();
    //loopTime = millisNow-lastTime;
    calculateCharge();
    calculatePower(millisNow);
    lastTime = millisNow;

    //calculate uptime move to display ever 100 statement
    upTime = (millisNow-fixRollover(wakeTime,millisNow))/1000L;
      
    if(Current>50){
      if(firstWakeup==true){
        wakeTime=millisNow;
        lastTime=wakeTime-1L;
        firstWakeup=false;
        Power = 0;
        turnDisplayOn(10);
      }
      //reset idle time
      idleTime = millisNow-2L;
    }
    
    chargeBatteries();

    
    if(Current<=50){
      //if waking up from sleep
      //if(firstWakeup==true){
        //test sleep duration
      }

      //if sitting for 3 seconds item was probably re plugged to clear data
      if((millisNow-fixRollover(idleTime,millisNow))>=8000){ 
        firstWakeup = true;
      }
      //if sitting for 10 seconds charging has probably finished doing nothing to sleep

      if((millisNow-fixRollover(idleTime,millisNow))>=10000){ 
        firstWakeup=true;
        if(charging==false){
          goSleep();
        }
      }
    
    if((millisNow-fixRollover(idleTime,millisNow))<10000){ 
      //update display every 100 loops
      i++;
      if (i>50){
        Display();
        i=0;
      }
    }
}
void chargeBatteries(){
  //handle checking if chargign, switching mosfets and display
  //check input voltage
  //Pro Mini analoge in is 3.3v tollerant
  //
  //B2----wwww---A3---wwww----GND
  //       R1          R2
  //5V  187.5k       300k
  //current through circuit = 10.246uA
  //(5.2/1023.0)= 5.083089mV resolution
  float latest = analogRead(A3)*0.005083089;
  inputVoltage = runningAverage(inputVoltage, latest, Valpha);
  
  
  if(inputVoltage>=4.8){
    if(firstWakeup){
      charging = true;
      chargeDisplay();
    }
    else{
      //screen already on charging a device
      errorString = "Charging";
      error++;
    }
  }
  if(inputVoltage<4.8){
     if(charging == true){
       charging = false;
     }
  }

  if(charging){
    mosfetControl();
  }
}
void chargeDisplay(){
  if(charging){
    if(screen==false){
      turnDisplayOn(200);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Charging Batteries");
    display.setCursor(0,16);
    display.println("Charge:" + String(Charge*100.0) + " %"); 
    display.setCursor(0,32);
    display.println("Input:" + String(inputVoltage) + " V"); 
    display.fillRoundRect(0,48, map(Charge,0,1,0,display.width()), 16, 4, WHITE);
    display.drawRoundRect(0,48, display.width(), 16, 4, WHITE);
    display.display();
    //delay(1000);
  }
}
void mosfetControl(){
  //switch mosfets balancing???
  if(mosfet){
    //do top battery
  }
  else{
    //do bottom battery
  }
}
void Display(){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    if(Current<1000){
      display.println("Current:" + String(Current) + "mA"); //Current
    }
    else{
      display.println("Current:" + String(Current/1000) + "A"); //Current
    }
    
    display.setCursor(0,16);
    if(Power>10000){
      display.println("Power:" + String(Power/1000.0) + "Ah");
    }
    else{
      display.println("Power:" + String(Power) + "mAh");
    }
    if(upTime/60%60%2){
      display.setCursor(0,32);
      display.println("V1:" + String(Battery1) +"V  V2:" + String(Battery2));
      //display.fillRoundRect(0,30, map(Charge,0,1,0,display.width()), 10, 4, WHITE);
      //display.drawRoundRect(0,30, display.width(), 10, 4, WHITE);
    }
    else{
      display.setCursor(0,32);
      display.println("Charge:" + String(Charge/10.0) + " %"); 
    }
    if(error>0 && upTime%2){
      display.setCursor(0,48);
      display.println(" " + String(errorString));
      error++;
      if(error>=5){
        error = 0;
        errorString ="";
      }
    }
    else{
      //if(upTime%2){
        int upTimeh = upTime/60/60;
        int upTimem = upTime/60%60;
        int upTimes = upTime%60;
        display.setCursor(0,48);
        display.println("Time on: " + String(upTimeh) + ":"+ String(upTimem) + ":"+ String(upTimes));
      //display.println("Voltage:" + String(Battery1+Battery2) + " V");
      //}
      //else{
      //  display.setCursor(0,48);
      //  display.println("V1:" + String(Battery1) + "V  V2:" + String(Battery2));
      //}
    }
    display.display();
    delay(100);
}

void measureVoltages(){
  //D1 Mini chip contains an inbuilt voltage divider
  //Vin---wwww---A0---wwww----Ain---wwww---GND 
  //       R3          R1             R2
  //      540k        220k -internal- 100k
  //current through circuit = 9.77uA
  //(8.6/1023.0)=0.008406647mV resolution
  //
  //Pro Mini analoge in is 3.3v tollerant
  //
  //B2----wwww---A0---wwww----GND
  //       R1          R2
  //4.2V  90.91k       300k
  //A0 = (R2/R1+R2)*B2 
  //current through circuit = 11uA
  //(4.3/1023.0)= 4.203324mV resolution
  float latest = analogRead(A0)*0.00420324;
  Battery1 = runningAverage(Battery1, latest, Valpha);
  
  
  //B1----wwww---A1---wwww----GND
  //       R1          R2
  //8.4V  320k         200k 
  //current through circuit = 16.538uA
  //(8.6/1023.0)=8.406647mV resolution
  latest = (analogRead(A1)*0.008406647)-latest; 
  //Voltage = Voltage - (Valpha * (Voltage - latest));
  Battery2 = runningAverage(Battery2, latest, Valpha);

  if(Battery1<3.5||Battery2<3.5){
    errorString = " Low Voltage";
    error++;
  }
  if(Battery1<3.3||Battery2<3.3){
    errorString = "Critical Voltage";
    error++;
  }
}

float runningAverage(float average, float latest, float alpha){
  average = average - (alpha * (average - latest));
  return average;
}

void measureCurrent(){
  //Pro Mini analog in is 3.3v tollerant
  //Vbat-----[load]----A2----wwww----GND
  //8.6v       I?           0.1ohms
  // I = V / R  3.3 / 0.1 = 33A max
  // V = I * R  A2 Voltage at 3Amps is 0.3V
  // (33/1023)=0.032258A 32.3mA resolution
  float latest = analogRead(A2)*32.258;
  Current = runningAverage(Current, latest, Calpha);
}

void calculateCharge(){
  float temp = (Battery1/Battery2);
  temp = abs(temp)-1;
  float total = (Battery1+Battery2)*100.0;
  float latest = map(total,661,841,0,1000.0); // /8.41V;
  Charge = runningAverage(Charge, latest, Palpha);
  if(Charge<-0.2){
    Charge = -0.2;
  }
   
  //if balance is out by 2%
  if(temp>0.02){
    errorString = " Balance Error";
    error++;
  }
}

void calculatePower(unsigned long millisNow){
  //calculate power mAh
  Power += Current*(millisNow-fixRollover(lastTime,millisNow))*0.000000278;// /60/60/1000;//0.00138889; // current amps *1000 to mA * change in time ms /1000/60/60 to hours
}

long fixRollover(unsigned long temp, unsigned long millisNow){
  if(temp>millisNow){ //fix rollover
    //need signed double resolution for this?
    //unsigned long fixed = (4294967294-temp)*-1;
    long fixed = ((4294967294-temp)*(-1));
    return fixed;
  }
  else{
    return temp;
  }
}
      

void turnDisplayOff(){
  screen = false;
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  digitalWrite(screenGroundPin, HIGH); //screen off
}
void turnDisplayOn(int delaytime){
  screen = true;
  digitalWrite(screenGroundPin, LOW); //screen on
  delay(delaytime);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  
}

void goSleep(){
  //if current = 0
  turnDisplayOff();
  //do the sleep thing
  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = _BV (WDCE) | _BV (WDE);
  // set interrupt mode and an interval 
  WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0);    // set WDIE, and 1 second delay
  wdt_reset();  // pat the dog

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  sleep_cpu ();

  // cancel sleep as a precaution
  sleep_disable();
}

