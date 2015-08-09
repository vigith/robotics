#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_SR.h> 


/**********************************/
/* --BEGIN HARDWARE CONNECTIONS-- */
/**********************************/


/* Shift-In Register 74HC165 (buttons) */
/* ----------------------------------- */
/* Connect pins A-H to 5V/GND/switches (it should be connected though) */
#define DATA_PIN 12  /* Pin 12 to SER_OUT (serial data out) */
#define CE_PIN   11  /* Pin 11 to !CE (clock enable, active low) */
#define SHLD_PIN 10  /* Pin 10 to SH/!LD (shift or active low load) */
#define CLK_PIN  9   /* Pin 9 to CLK (the clock that times the shifting) */

/* Shift-Out Register 74HC595 */
/* -------------------------- */
/* LCD */
#define LCD_DATA_PIN   2        /* Pin 2 to SER_IN  */
#define LCD_CLOCK_PIN  3        /* Pin 3 to Clock */
#define LCD_ENABLE_PIN 4        /* Pin 4 to L_Clock (Latch) */

/* Motors */
/* ------ */
#define MOTOR_RIGHT_SPEED 5  /* Pin 5  analogWrite, EnablePin (Pin1 L293D) */
#define MOTOR_RIGHT_DIR1  7  /* (Pin2 L293D) */
#define MOTOR_RIGHT_DIR2  8

#define MOTOR_LEFT_SPEED 6   /* Pin 8 analogWrite, EnablePin (Pin9 L293D) */
#define MOTOR_LEFT_DIR1  0   /* (Pin10 L293D) */
#define MOTOR_LEFT_DIR2  1


/********************************/
/* --END HARDWARE CONNECTIONS-- */
/********************************/


#define DEBUG     0             /* turn on debugging */
#define MAX_GEAR  5             /* max gear allowed */
#define SPEED(x)  (x * 255/MAX_GEAR) /* speed calculated from gear (255 is the max speed, PWM) */


/* Button Definitions */
#define GEAR_UP     0        /* A [Button 1] (Gear Up) */
#define GEAR_DOWN   1        /* A [Button 2] (Gear Down) */
#define FWD_RIGHT   2        /* C [Button 3] (Forward Right Belt) */
#define FWD_LEFT    3        /* D [Button 4] (Forward Left Belt) */
#define REV_RIGHT   4        /* E [Button 5] (Reverse Right Belt) */
#define REV_LEFT    5        /* F [Button 6] (Reverse Left Belt ) */


#define GEAR_UP_PRESS   0        /* B [Button 1] (Gear Up Press Event) */
#define GEAR_DOWN_PRESS 1        /* B [Button 2] (Gear Down Press Event) */

byte buttons; // Variable to store the 8 values loaded from the shift register
byte gear_press;   /* did we press the button */

short int gear;                /* gear for belt speed */

// constructor prototype parameter:
//  LiquidCrystal_SR lcd(DataPin, ClockPin, EnablePin);
LiquidCrystal_SR lcd(LCD_DATA_PIN, LCD_CLOCK_PIN, LCD_ENABLE_PIN); 

void setup() {                
  // Initialize serial to gain the power to obtain relevant information, 9600 baud
  //  Serial.begin(9600);

  pinMode(MOTOR_RIGHT_SPEED, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR1, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR2, OUTPUT);

  pinMode(MOTOR_LEFT_SPEED, OUTPUT);
  pinMode(MOTOR_LEFT_DIR1, OUTPUT);
  pinMode(MOTOR_LEFT_DIR2, OUTPUT);

  /* Shift-In */
  // Initialize each digital pin to either output or input
  // We are commanding the shift register with each pin with the exception of the serial
  // data we get back on the data_pin line.
  pinMode(SHLD_PIN, OUTPUT);
  pinMode(CE_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(DATA_PIN, INPUT);

  // Required initial states of these two pins according to the datasheet timing diagram
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(SHLD_PIN, HIGH);

  /* LCD */
  lcd.begin ( 16, 2 );
}

// A DEBUG function that prints all the 1's and 0's of a byte
void print_byte(byte val) {
  //  Serial.println("The incoming values of the shift register (ABCDEFGH): ");

  for(byte i=0; i<=7; i++) {
    Serial.print(val >> i & 1, BIN); // Magic bit shift, if you care look up the <<, >>, and & operators
  }
  Serial.print("\n");

  return;
}

// This code is intended to trigger the shift register to grab values from it's A-H inputs
byte read_shiftin() {
  byte the_shifted = 0;  // An 8 bit number to carry each bit value of A-H

  // Trigger loading the state of the A-H data lines into the shift register
  digitalWrite(SHLD_PIN, LOW);
  digitalWrite(SHLD_PIN, HIGH);

  // Required initial states of these two pins according to the datasheet timing diagram
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(CE_PIN, LOW); // Enable the clock

  // Get the A-H values
  the_shifted = shiftIn(DATA_PIN, CLK_PIN, MSBFIRST);
  digitalWrite(CE_PIN, HIGH); // Disable the clock

  return the_shifted;

}

/* Run The Motor! (2 motors, Left and Right)
   for running motor we have 2 variables: speed (1 pin, analog), direction (2 pins) */
void run_motor() {
  /* if forward is pressed, we don't care whether reverse is pressed or not */

  /* Right Belt  */
  /* forward or reverse (2,4) */
  int right_fwd = !(buttons & (1 << FWD_RIGHT)) ? HIGH : /* is forward set (ie, bit is clear, button is pressed) */
    !(buttons & (1 << REV_RIGHT)) ? LOW : /* is reverse set */
    -1;       /* if none is set, return -1 */
  /* if right_fwd == -1, stop the right belt, else calculate from SPEED macro */
  int right_speed = right_fwd == -1 ? 0 : SPEED(gear); /* analog value is from 0 to 255 */
  
  /* Left Belt  */
  /* forward or reverse */
  int left_fwd = !(buttons & (1 << FWD_LEFT)) ? HIGH : /* is forward set (ie, bit is clear, button is pressed) */
    !(buttons & (1 << REV_LEFT)) ? LOW : /* is reverse set */
    -1;                                /* if none is set, return -1 */

  /* if left_fwd == -1, stop the left belt, else calculate from SPEED macro */
  int left_speed = left_fwd == -1 ? 0 : SPEED(gear); /* analog value is from 0 to 255 */

  analogWrite(MOTOR_LEFT_SPEED, left_speed); /* speed */
  digitalWrite(MOTOR_LEFT_DIR1, left_fwd);   /* direction */
  digitalWrite(MOTOR_LEFT_DIR2, !left_fwd);  /* inverse the direction (1,0) -> forward; (0,1) -> backward */
  //    digitalWrite(0, 0);   /* direction */
  // digitalWrite(1, 1);   /* direction */

  analogWrite(MOTOR_RIGHT_SPEED, right_speed); /* speed */
  digitalWrite(MOTOR_RIGHT_DIR1, right_fwd);   /* direction */
  digitalWrite(MOTOR_RIGHT_DIR2, !right_fwd);  /* inverse the direction (1,0) -> forward; (0,1) -> backward */

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(right_speed);
  lcd.setCursor(9, 0); lcd.print(left_speed);
  lcd.setCursor(0, 1); lcd.print(gear);
  lcd.setCursor(4, 1); lcd.print(left_fwd);
  lcd.setCursor(9, 1); lcd.print(right_fwd);

  return;
}

/* if there is a gear change event, change gear */
void change_gear() {
  /* UP GEAR */
  /* if we have a gear event, change speed */
  if (buttons & (1 << GEAR_UP)) { /* Gear is Open (mark the latch saying, button is open) */
    gear_press |= (1 << GEAR_UP_PRESS);     /* mark the open latch */
  } else if(!(buttons & (1 << GEAR_UP)) && gear_press & (1 << GEAR_UP_PRESS)) { /* (test bit clear) press is no more high (ie, some pressed: pullup inverse logic), but gate is marked */
    /* now we know we had a press event (release is marked high but press is low) */
    if (gear < MAX_GEAR)
      gear++;
      /* debug */
    if (DEBUG) Serial.println(gear);        
    gear_press &= ~(1 << GEAR_UP_PRESS); /* remove the open latch mark (ie, someone pressed) (1 event cycle is over) */
  }
  
  /* DOWN GEAR */
  /* if we have a gear event, change speed */
  if (buttons & (1 << GEAR_DOWN)) { /* Gear is Open (mark the latch saying, button is open) */
    gear_press |= (1 << GEAR_DOWN_PRESS);     /* mark the open gate */
  } else if(!(buttons & (1 << GEAR_DOWN)) && gear_press & (1 << GEAR_DOWN_PRESS)) { /* (test to see whether bit clear, clear means button is pressed) */
    /* now we know we had a press event (release is marked high but press is low) */
    if (gear > 0)
      gear--;
    /* debug */
    if (DEBUG) Serial.println(gear);
    gear_press &= ~(1 << GEAR_DOWN_PRESS); /* remove the open latch mark (ie, someone pressed) (1 event cycle is over) */
  }

  return;
}

// The part that runs to infinity and beyond
void loop() {
  buttons = read_shiftin(); // Read the shift register (state of the buttons)

  if (DEBUG) {
    // Print out the values being read from the shift register
    print_byte(buttons); // Print every 1 and 0 that correlates with A through H
  }

  /* gear change */
  change_gear();                /* if no gear change event is there, this will be a noop */

  /* run the motor */
  run_motor();                  /* run the motor based on info from button state + gear */

  // delay(1000); // Wait for some arbitrary amount of time

}
