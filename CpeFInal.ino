//Authors: Trent Smelich, Jia Xing Luo, Luis Garcia
//Date: 5/11/24
#include <dht.h> 
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Stepper.h>
#include "RTClib.h"
#define RDA 0x80
#define TBE 0x20


char disabledState = 'd';
char idleState = 'i';
char errorState = 'e';
char runningState = 'r';
char resetState = 'x';

char currentState = 'd';
char prevState = 'd';
bool machineDisabled = true;
bool t = false;


#define WATER_LEVEL_THRESHOLD 10
#define TEMP_THRESHOLD 20

RTC_DS1307 rtc;

unsigned long previousMillis = 0;
const long interval = 60000;



const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 52, 50, 48, 46);

volatile unsigned char *portDDRH = (unsigned char *) 0x101;
volatile unsigned char *portH = (unsigned char *) 0x102;

volatile unsigned char *portDDRA = (unsigned char *) 0x21; 
volatile unsigned char *portA =    (unsigned char *) 0x22; 
volatile unsigned char *pinA =    (unsigned char *) 0x20; 

volatile unsigned char *portDDRC = (unsigned char *) 0x27; 
volatile unsigned char *portC =    (unsigned char *) 0x28; 
volatile unsigned char *pinC =    (unsigned char *) 0x26; 

volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB =    (unsigned char *) 0x25;

volatile unsigned char *portDDRG = (unsigned char *) 0x33;
volatile unsigned char *portG =    (unsigned char *) 0x34;
 
volatile unsigned char *portDDRE = (unsigned char *) 0x2D;
volatile unsigned char *portE =    (unsigned char *) 0x2E;

volatile unsigned char* portD = (unsigned char*) 0x2B;
volatile unsigned char* portDDRD = (unsigned char*) 0x2A;
volatile unsigned char* pinD = (unsigned char*) 0x29;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;


dht DHT;
#define DHT11_PIN 7

bool machine = true;

int value = 0;
void setup() {
  // put your setup code here, to run once:
  U0init(9600);
  lcd.begin(16,2);
  adc_init();

  *portDDRH |= (1 << 6);
  *portH &= ~(1 << 6);
  

  *portDDRA |= (1 << 0);
  *portA &= ~(1 << 0);
  *portDDRH |= (1 << 3);
  *portDDRH |= (1 << 5);
  
  *portDDRG |= (1<<5);// portG5
  *portDDRE |= (1<<5);// portE5



  *portDDRC |= (1 << 7); //red pc7 30
  *portDDRC |= (1 << 5);  //green pc5 32
  *portDDRC |= (1 << 3); //blue pc3 34
  *portDDRC |= (1 << 1);  //yellow pc1 36

  *portDDRC &= ~(1 << 2); //pc2 35 input
  *portDDRC &= ~(1 << 4); //pc4 33 input
  *portDDRC &= ~(1 << 6); //pc6 31 input
  *portDDRC &= ~(1 << 0); //pc0 37 input
  rtc.begin();
}

void loop() {
  
    if(!machineDisabled){
      unsigned long currentMillis = millis();
      int chk = DHT.read11(DHT11_PIN);

      if(currentMillis - previousMillis >= interval){
        previousMillis = currentMillis;
        //displays hum and temp to lcd
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Temperature:");
        lcd.print(DHT.temperature);
        lcd.setCursor(0,1);
        lcd.print("Humidity:");
        lcd.print(DHT.humidity);
      }
      
    //●Monitor the water levels in a reservoir and print an alert when the level is too low
    // put your main code here, to run repeatedly:
      *portH |= 0x01 << 6;
      value = adc_read(0);
      *portH &= ~(1 << 6);  

      //Monitor and display the current air temp and humidity on an LCD screen.
      

      prevState = currentState;
      currentState = selectState(currentState, DHT.temperature, value);
      
      if(prevState != currentState){
        //print time stamp and state change
        U0putchar('S');
        U0putchar('t');
        U0putchar('a');
        U0putchar('t');
        U0putchar('e');
        U0putchar(' ');
        U0putchar('C');
        U0putchar('h');
        U0putchar('a');
        U0putchar('n');
        U0putchar('g');
        U0putchar('e');
        U0putchar(':');


        recordTime();
      }
      //dont use serial
      if(currentState == errorState){
        //red led on
        *portC &= ~(1 << 5);

        *portC |= (1 << 7);

        *portC &= ~(1 << 1);

        *portC &= ~(1 << 3);
        //fan off
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Water level");
        lcd.setCursor(0,1);
        lcd.print("is too low.");
        //doing stepper
        //monitor water
        unsigned char resetButton = (*pinC >> 0) & 0x01;
        if(resetButton == 1){
          currentState = resetState;
          lcd.clear();
        }
      }

      if(currentState == idleState){
        //fan off
        //green led on
        *portC |= (1 << 5);

        *portC &= ~(1 << 7);

        *portC &= ~(1 << 1);

        *portC &= ~(1 << 3);
        //display temp humid
        //doing stepper
        //monitor water
      }

      if(currentState == runningState){
        *portC &= ~(1 << 5);

        *portC &= ~(1 << 7);

        *portC &= ~(1 << 1);

        *portC |= (1 << 3);
        //fan on
        //display temp humid
        //doing stepper //on entry is start fan motor, on exit is stop fan motor
        *portH |= (1 << 3);
      //● Start and stop a fan motor as needed when the temperature falls out of a specified range
      //(high or low).
      // Turn off pin 5 of port H
        *portH &= ~(1 << 5);
        //monitor water
      }else{
        //MAKE SURE TO TURN FAN OFF
        *portH &= ~(1<<3); 
        *portH &= ~(1<<5);
      }

    
      

    //● Allow a user to use a control to adjust the angle of an output vent from the system
      unsigned char inputLeft = (*pinC >> 4) & 0x01;
      unsigned char inputRight = (*pinC >> 6) & 0x01;

      if(inputLeft == 1){
        myStepper.setSpeed(20);
        myStepper.step(-stepsPerRevolution);
      }
      if(inputRight == 1){
        myStepper.setSpeed(20);
        myStepper.step(stepsPerRevolution);
      }
    }
    else{
      //when machine disabled
      lcd.clear();
      //turn yellow led on
      *portC |= (1 << 1);
      *portC &= ~(1 << 3);
      *portC &= ~(1 << 5);
      *portC &= ~(1 << 7);

    }
  //● Allow a user to enable or disable the system using an on/off button
  
  attachInterrupt(digitalPinToInterrupt(19), onOff, RISING);
  if(t){
    
    recordTime();
    t = false;
  }
  
  //● Record the time and date every time the motor is turned on or off. This information
  //should be transmitted to a host computer (over USB)
  
}


void recordTime(){
  DateTime now = rtc.now();
    int year = now.year(), month = now.month(), day = now.day(), hour = now.hour(), min = now.minute(), sec = now.second() ;
    U0putchar(year/1000 + '0');
    year = year % 1000;
    U0putchar(year/100 + '0');
    year = year % 100;
    U0putchar(year/10 + '0');
    year = year % 10;
    U0putchar(year + '0');
    U0putchar('/');
    U0putchar(month/10 + '0');
    month = month % 10;
    U0putchar(month + '0');
    U0putchar('/');
    U0putchar(day/10 + '0');
    day = day % 10;
    U0putchar(day + '0');
    U0putchar(' ');
    U0putchar('(');
    U0putchar(')');
    U0putchar(' ');
    U0putchar(hour/10 + '0');
    hour = hour % 10;
    U0putchar(hour + '0');
    U0putchar(':');
    U0putchar(min/10 + '0');
    min = min % 10;
    U0putchar(min + '0');
    U0putchar(':');
    U0putchar(sec/10 + '0');
    sec = sec % 10;
    U0putchar(sec + '0');
    U0putchar('\n');
    
    
}


void onOff(){
  if(machineDisabled){
    currentState = idleState;
    machineDisabled = false;
    U0putchar('O');
    U0putchar('N');
    U0putchar(':');

  }
  else{
    currentState = disabledState;
    machineDisabled = true;
    U0putchar('O');
    U0putchar('F');
    U0putchar('F');
    U0putchar(':');
  }
  t = true;
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

//serial library

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}




char selectState(char state, int temp, int level){
  char s;
  if(temp <= TEMP_THRESHOLD && state == runningState){
    s = idleState;
  }
  else if(temp > TEMP_THRESHOLD && state == idleState){
    s = runningState;
  }
  else if(level < WATER_LEVEL_THRESHOLD && state == runningState){
    s = errorState;
  }
  else if(state == resetState && level > WATER_LEVEL_THRESHOLD){
    s = idleState;
  }
  else if(state == resetState && level < WATER_LEVEL_THRESHOLD){
    s = errorState;
  }
  else{
    s = state;
  }
  return s;
}









