/* Rotory Encoder to Camera
      By Steve Groves March 2014
      This is version 1.6 and uses a 16x2 LCD screen
   The rotary encoder is used to enter two valuse (delay time and shutter time and pics)
   The switch on the rotary encoder is the enter key
   The lcd displays the values
   The led's on the rotary encoder flash three times just before the picture is taken.
   
   April 2015 Added the ability to set the number of pictures to be taken (Pic's)
*/
   
enum PinAssignments {
  encoderPinA = 2,   // rigth
  encoderPinB = 3,   // left
  clearButton = 10,   // button on the encoder
  ledPin = A0        // LED pin
};

#include <LiquidCrystal.h>
#include <Wire.h>
#include <avr/power.h>
LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

// general variables
volatile unsigned int encoderPos = 0;  // a counter for the dial
unsigned int lastReportedPos = 1;   // change management
static boolean rotating=false;      // debounce management
unsigned long value[3];              //Stored values (dTime and sTime and no. of picture
byte counter = 0;
boolean loopCounter = false;        // flag to prevent re-entry after entering values
const byte focus = 8;
const byte shutter = 9;

char* advert = ("Control System  SG/2014");

// interrupt service routine vars
boolean A_set = false;              
boolean B_set = false;

// set up the pins, display and interrupts
void setup() {
  power_twi_disable();     // Used to save power
  power_timer1_disable();  // these are not used 
  power_timer2_disable();  // in the cct.
  power_usart0_disable();
  
  pinMode(ledPin, OUTPUT);
  pinMode(encoderPinA, INPUT); 
  pinMode(encoderPinB, INPUT); 
  pinMode(clearButton, OUTPUT);
 // turn on pullup resistors
  digitalWrite(encoderPinA, HIGH);
  digitalWrite(encoderPinB, HIGH);
  digitalWrite(ledPin, HIGH);
  
  pinMode(focus, OUTPUT); 
  pinMode(shutter, OUTPUT);
  
  lcd.begin(16, 2); 

  screenAdvert();
  
  screenSetup();

  attachInterrupt(0, doEncoderA, CHANGE); // encoder pin on interrupt 0 (pin 2)
  attachInterrupt(1, doEncoderB, CHANGE); // encoder pin on interrupt 1 (pin 3)
}

// main loop
void loop() { 
  rotating = true;  // reset the debouncer
  digitalWrite(ledPin, HIGH);  // turn on the encoder led

  if (lastReportedPos != encoderPos && counter < 3) // only go into this if when conditions are met
    {
    loopCounter = false; // make sure we dont re-enter the next IF condition
    if (counter == 0) 
      {
      lcd.setCursor(7,0); lcd.blink(); lcd.print(encoderPos, DEC);
        if (encoderPos == 0)  {lcd.setCursor(8,0); lcd.print("    ");} // added to clear training zeros
        if (encoderPos < 10)  {lcd.setCursor(8,0); lcd.print(" ");}    // when number is decreasing
        if (encoderPos < 100) {lcd.setCursor(9,0); lcd.print(" ");}
      } 
     if (counter == 1) 
      {    
      lcd.setCursor(9,1); lcd.blink(); lcd.print(encoderPos, DEC);
        if (encoderPos == 0)  {lcd.setCursor(10,1); lcd.print("    ");} // added to clear training zeros
        if (encoderPos < 10)  {lcd.setCursor(10,1); lcd.print(" ");}    // when number is decreasing
        if (encoderPos < 100) {lcd.setCursor(11,1); lcd.print(" ");}
      }
     if (counter == 2) 
      {    
      screenSetup2();   //Scroll screen up to set number of pictures
      lcd.setCursor(7,1); lcd.blink(); lcd.print(encoderPos, DEC);
        if (encoderPos == 0)  {lcd.setCursor(8,1); lcd.print("    ");} // added to clear training zeros
        if (encoderPos < 10)  {lcd.setCursor(8,1); lcd.print(" ");}    // when number is decreasing
        if (encoderPos < 100) {lcd.setCursor(9,1); lcd.print(" ");}
      }
    lastReportedPos = encoderPos;
    }
  if (digitalRead(clearButton) == HIGH && loopCounter == false && encoderPos != 0 && counter < 3)  
   {
    value[counter] = encoderPos; //make value[counter] hold current position
    digitalWrite(ledPin, LOW);   // Turn off led
    encoderPos = 0;              // reset the encoder to 0   
    delay(150);                  // keep led on so we see it
    loopCounter = true;          // Flag to stop you re-entering this loop
    counter ++;
    if (counter > 2)             // Once the third value has been entered show the values
      {
        lcd.noBlink();
        cameraTrigger();
      }   
    }
  
  if (value[2] > 0)
    {
      lcd.noBlink();
      cameraTrigger();
    }
}

void cameraTrigger()
{
  lcd.noCursor();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("D="); lcd.print(value[0]); // print delay time to lcd
  lcd.setCursor(9,0); lcd.print("P="); lcd.print(value[2]); // print shutter time to lcd
  lcd.setCursor(0,1); lcd.print("Waiting......");  
  delay(value[0]*1000);       // wait the delay time
  lcd.setCursor(0,1); lcd.print("Shooting.....");
  value[2] -=1;

  flashled();
  
  digitalWrite(focus, HIGH);               // allow the camera to focus
  delay(1000);                             
  
  digitalWrite(shutter, HIGH);             // open the shutter
  delay(value[1]*1000);                    // wait the shutter time
  digitalWrite(shutter, LOW);              // close the shutter
  
  delay(50);
  digitalWrite(focus, LOW);                // release focus control
  digitalWrite(ledPin, LOW);               // turn off the led
  
  if (value[2] == 0)
    {
      finishScreen();
    }
}

// Interrupt on A changing state
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done

  // Test transition, did things really change? 
  if( digitalRead(encoderPinA) != A_set ) // debounce once more
    {  
    A_set = !A_set;
    // adjust counter +1 if A leads B
    if ( A_set && !B_set ) 
      encoderPos += 1;

    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(encoderPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if( B_set && !A_set ) 
      encoderPos -= 1;

    rotating = false;
  }
}

void flashled()
{
  for (byte ledflash = 0; ledflash < 4; ledflash ++)
  {
    digitalWrite(ledPin, LOW);
    delay(200);
    digitalWrite(ledPin, HIGH); 
    // keeps led on while taking picture
    delay(200);
  }
}

void resetVariables()
{
  counter = 0;
  loopCounter = false;
}

void screenAdvert()
{
  lcd.home();
  for (int scroll = 16; scroll > -17; scroll--)
  {
    lcd.setCursor(scroll,0);
    lcd.print(advert);
    lcd.print(" ");
    delay(200);
  }
  delay(1000);
  lcd.clear();
}

void screenSetup()
{
  lcd.setCursor(0,0); lcd.print("Delay= ");
  lcd.setCursor(0,1); lcd.print("Shutter= ");
} 

void screenSetup2()
{
  lcd.setCursor(0,0); lcd.print("Shutter= "); lcd.print(value[1]); lcd.print("    ");
  lcd.setCursor(0,1); lcd.print("Pic's= ");
}

void finishScreen()
{
  lcd.setCursor(0,0); lcd.print("Finished taking ");
  lcd.setCursor(0,1); lcd.print("the pictures ");
}
