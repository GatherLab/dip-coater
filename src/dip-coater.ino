#include "LiquidCrystal.h"
#include <AccelStepper.h>
#include <EEPROM.h>         // Library to save in Arduino permanent storage (eeprom)
#include "EncoderTool.h"

using namespace EncoderTool;

const int rs = 12, en = 11, d4 = 4, d5 = 5, d6 = 6, d7 = 7;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Definitions:
#define DISPLAY_ROWS      2
#define PER_SPACE         1

#define KEYPAD_ROWS       4
#define KEYPAD_COLS       4

// Pins for Rotary encoder
#define re_clk 8
#define re_data 9
#define re_pushbutton 10

// Stepper motor pins
#define sm_step 2
#define sm_dir 3
#define sm_sleep A5

#define sm_m0 A1
#define sm_m1 A2
#define sm_m2 A3

// For Menu
unsigned char selected = 1;
unsigned char prev_key;

// Rotary Encoder 
int previous_encoder_value = 0;
PolledEncoder enc;

// Stepper motor
int no_microsteps = 1;
int available_microsteps[] = {1,2,4,8,16,32};
int no_of_different_microsteps = 6;

// Define the stepper motor and the pins that is connected to
AccelStepper stepper1(2, sm_step, sm_dir); // (Typeof driver: with 2 pins, STEP, DIR)
long max_position = -55000;
long min_position = 0;

const int bottom_stop_switch = 13;

// Struct to safe programs in EEPROM
struct CoatingProgram{
  long bottom_position;
  long top_position;
  long velocity;
  long acceleration;
  long waiting_time;
};

// Menu struct
typedef const struct MenuStructure
{
  const char *text;
  unsigned char num_menupoints;
  unsigned char up;
  unsigned char down;
  unsigned char enter;
  void (*fp) (void);
} MenuEntry;

// For right depiction, the number of characters has to match the width of the display (16 in my case)
const char menu_000[] = " Menu:          ";    // 0
const char menu_001[] = "  Manual        ";    // 1
const char menu_002[] = "  Program 1     ";    // 2
const char menu_003[] = "  Program 2     ";    // 3
const char menu_004[] = "  Program 3     ";    // 4
const char menu_005[] = "  Settings      ";    // 5

const char menu_100[] = " Program 1:     ";   // 6
const char menu_101[] = "  Run           ";   // 7
const char menu_102[] = "  Bottom Pos.   ";   // 8
const char menu_103[] = "  Top Pos.      ";   // 9
const char menu_104[] = "  Speed         ";   // 10
const char menu_105[] = "  Acceleration  ";   // 11
const char menu_106[] = "  Waiting Time  ";   // 12
const char menu_107[] = "  Back          ";   // 13

const char menu_200[] = " Program 2:     ";   // 14
const char menu_201[] = "  Run           ";   // 15
const char menu_202[] = "  Bottom Pos.   ";   // 16
const char menu_203[] = "  Top Pos.      ";   // 17
const char menu_204[] = "  Speed         ";   // 18
const char menu_205[] = "  Acceleration  ";   // 19
const char menu_206[] = "  Waiting Time  ";   // 20
const char menu_207[] = "  Back          ";   // 21

const char menu_300[] = " Program 3:     ";   // 22
const char menu_301[] = "  Run           ";   // 23
const char menu_302[] = "  Bottom Pos.   ";   // 24
const char menu_303[] = "  Top Pos.      ";   // 25
const char menu_304[] = "  Speed         ";   // 26
const char menu_305[] = "  Acceleration  ";   // 27
const char menu_306[] = "  Waiting Time  ";   // 28
const char menu_307[] = "  Back          ";   // 29

const char menu_400[] = " Settings:      ";   // 30
const char menu_401[] = "  Calibrate     ";   // 31
const char menu_402[] = "  Move to Bottom";   // 32
const char menu_403[] = "  Move to Top   ";   // 33
const char menu_404[] = "  Microsteps    ";   // 34
const char menu_405[] = "  Back          ";   // 35

void start(void);
void show_menu(void);
void calibrate_motor();
void manually_move();
void change_bottom();
void change_top();
void change_speed();
void change_acceleration();
void change_waiting_time();
void run_dip();
void move_to_bottom();
void move_to_top();
void change_microsteps();

MenuEntry menu[] =
{
  // text, num_menupoints, up, down, enter, *fp (refers to a function in the arduino code)
  {menu_000, 6, 0, 0, 0, 0},        // 0
  {menu_001, 6, 1, 2, 1, manually_move},        // 1
  {menu_002, 6, 1, 3, 7, 0},       // 2
  {menu_003, 6, 2, 4, 15, 0},       // 3
  {menu_004, 6, 3, 5, 23, 0},       // 4
  {menu_005, 6, 4, 5, 31, 0},       // 5

  {menu_100, 8, 0, 0, 0, 0},        // 6
  {menu_101, 8, 7, 8, 7, run_dip},     // 7
  {menu_102, 8, 7, 9, 8, change_bottom},     // 8
  {menu_103, 8, 8, 10, 9, change_top},     // 9
  {menu_104, 8, 9, 11, 10, change_speed},     // 10
  {menu_105, 8, 10, 12, 11, change_acceleration},      // 11
  {menu_106, 8, 11, 13, 12, change_waiting_time},      // 12
  {menu_107, 8, 12, 13, 1, 0},      // 13
  
  {menu_200, 8, 0, 0, 0, 0},                  // 14
  {menu_201, 8, 15, 16, 15, run_dip},       // 15
  {menu_202, 8, 15, 17, 16, change_bottom},       // 16
  {menu_203, 8, 16, 18, 17, change_top},     // 17
  {menu_204, 8, 17, 19, 18, change_speed},        // 18
  {menu_205, 8, 18, 20, 19, change_acceleration},                // 19
  {menu_206, 8, 19, 21, 20, change_waiting_time},      // 20
  {menu_207, 8, 20, 21, 1, 0},                // 21
  
  {menu_300, 8, 0, 0, 0, 0},                  // 22
  {menu_301, 8, 23, 24, 23, run_dip},       // 23
  {menu_302, 8, 23, 25, 24, change_bottom},       // 24
  {menu_303, 8, 24, 26, 25, change_top},  // 25
  {menu_304, 8, 25, 27, 26, change_speed},        // 26
  {menu_305, 8, 26, 28, 27, change_acceleration},                // 27
  {menu_306, 8, 27, 29, 28, change_waiting_time},      // 28
  {menu_307, 8, 28, 29, 1, 0},                // 29
  
  {menu_400, 6, 0, 0, 0, 0},                  // 30
  {menu_401, 6, 31, 32, 31, calibrate_motor},       // 31
  {menu_402, 6, 32, 33, 32, move_to_bottom},       // 32
  {menu_403, 6, 33, 34, 33, move_to_top},       // 33
  {menu_404, 6, 34, 35, 34, change_microsteps},       // 34
  {menu_405, 6, 35, 35, 1, 0},       // 35
};



void setup() {
  // Define encoder
  enc.begin(re_clk, re_data, re_pushbutton);

  // Define pins for DRV8825 driver
  pinMode (sm_sleep,OUTPUT);  
  pinMode (sm_m0,OUTPUT);
  pinMode (sm_m1,OUTPUT);
  pinMode (sm_m2,OUTPUT);

  // Set the microstepping to 1 for the start
  digitalWrite(sm_m0, LOW);
  digitalWrite(sm_m1, LOW);
  digitalWrite(sm_m2, LOW);

  pinMode(bottom_stop_switch, INPUT);

  // Do initial calibration to check for zero position
  calibrate_motor();

  // Clear LCD
  lcd.clear();
  lcd.begin(16, 2);

  // Show menu on LCD display
  show_menu();
  
  // Serial monitor
  Serial.begin(9600);
  Serial.println("Setup DONE");
}

void loop() {
   enc.tick();
   // If the previous and the current state of the clock are different, that means a step has occured
   if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
        selected = menu[selected].up;
      }
      else if (enc.getValue() > previous_encoder_value)
      {
        selected = menu[selected].down;
      }
      previous_encoder_value = enc.getValue();
      show_menu();
   }
   if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
        if (menu[selected].fp != 0)
        {
          menu[selected].fp();
        }
        prev_key = selected;
        selected = menu[selected].enter;
        show_menu();
     }
   }
}

void start()
{
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("start works!");

    delay(1000);   
}

void run_dip(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 7){
    eprom_position = 0;
  }
  else if (selected == 15){
    eprom_position = 100;
  }
  else if (selected == 23){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moving ...");

  // Set acceleration and speed
  stepper1.setAcceleration(temp.acceleration);
  stepper1.setMaxSpeed(temp.velocity);

  // Move to top position
  safe_move_stepper(-1 * temp.top_position);
  delay(500);

  // Move to bottom
  safe_move_stepper(-1 * temp.bottom_position);
  delay(temp.waiting_time);

  // Move back up
  safe_move_stepper(-1 * temp.top_position);

  // Resume back to menu
  show_menu(); 
    
}

void manually_move() {

  // If value gets smaller it can reduce its number of digits
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clockwise: Down ");
  
  stepper1.setAcceleration(5000); // Set acceleration value for the stepper
  
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
        safe_move_stepper(stepper1.currentPosition() + 1000 * no_microsteps);
        lcd.setCursor(0,1);
        lcd.print(F("                "));
      }
      else if (enc.getValue() > previous_encoder_value)
      {
        safe_move_stepper(stepper1.currentPosition() - 1000 * no_microsteps);
      }
      previous_encoder_value = enc.getValue();
      
      lcd.setCursor(0, 1);
      lcd.print(abs(stepper1.currentPosition()));
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
      // Resume back to menu
      show_menu();  
      break;
     }
    }
  }
}

void change_bottom(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 8){
    eprom_position = 0;
  }
  else if (selected == 16){
    eprom_position = 100;
  }
  else if (selected == 24){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bottom Position ");
  lcd.setCursor(0, 1);
  lcd.print(temp.bottom_position);

  // Let the user change the value of the bottom position with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
        if (temp.bottom_position < abs(max_position) * no_microsteps){
          temp.bottom_position += 100;
        }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (temp.bottom_position > 0){
            // If value gets smaller it can reduce its number of digits
            lcd.setCursor(0,1);
            lcd.print(F("                "));
            
            temp.bottom_position -= 100;
          }
      }
      previous_encoder_value = enc.getValue();

      // Display changed value
      lcd.setCursor(0, 1);
      lcd.print(temp.bottom_position);
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
      // Save new value in EEPROM
      EEPROM.put(eprom_position, temp);
       
      // Resume back to menu
      show_menu();  
      break;
     }
    }
  }
}

void change_top(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 9){
    eprom_position = 0;
  }
  else if (selected == 17){
    eprom_position = 100;
  }
  else if (selected == 25){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Top Position    ");
  lcd.setCursor(0, 1);
  lcd.print(temp.top_position);
  
  // Let the user change the value of the bottom position with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
          if (temp.top_position < abs(max_position) * no_microsteps){
            temp.top_position += 100;
          }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (temp.top_position > 0){
            // If value gets smaller it can reduce its number of digits
            lcd.setCursor(0,1);
            lcd.print(F("                "));
            temp.top_position -= 100;
          }
      }
      previous_encoder_value = enc.getValue();

       // Display changed value
      lcd.setCursor(0, 1);
      lcd.print(temp.top_position);
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
      // Save new value in EEPROM
      EEPROM.put(eprom_position, temp);
       
      // Resume back to menu
      show_menu();  
      break;
     }
    }
  }
}

void change_speed(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 10){
    eprom_position = 0;
  }
  else if (selected == 18){
    eprom_position = 100;
  }
  else if (selected == 26){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Speed           ");
  lcd.setCursor(0, 1);
  lcd.print(temp.velocity);
  
  // Let the user change the value of the bottom position with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
          if (temp.velocity < 10000 * no_microsteps){
            temp.velocity += 100;
          }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (temp.velocity > 0){
            // If value gets smaller it can reduce its number of digits
            lcd.setCursor(0,1);
            lcd.print(F("                "));
            temp.velocity -= 100;
          }
      }
      previous_encoder_value = enc.getValue();

      // Display changed value
      lcd.setCursor(0, 1);
      lcd.print(temp.velocity);
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
      // Save new value in EEPROM
      EEPROM.put(eprom_position, temp);
       
      // Resume back to menu
      show_menu();  
      break;
     }
    } 
  }
}

void change_acceleration(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 11){
    eprom_position = 0;
  }
  else if (selected == 19){
    eprom_position = 100;
  }
  else if (selected == 27){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acceleration    ");
  lcd.setCursor(0, 1);
  lcd.print(temp.acceleration);
  
  // Let the user change the value of the bottom position with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
          if (temp.acceleration < 10000 * no_microsteps){
            temp.acceleration += 100;
          }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (temp.acceleration > 0){
            // If value gets smaller it can reduce its number of digits
            lcd.setCursor(0,1);
            lcd.print(F("                "));
            
            temp.acceleration -= 100;
          }
      }
      previous_encoder_value = enc.getValue();

      // Display changed value
      lcd.setCursor(0, 1);
      lcd.print(temp.acceleration);
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
       // Save new value in EEPROM
      EEPROM.put(eprom_position, temp);
       
      // Resume back to menu
      show_menu();  
      break;
     }
    } 
  }
}

void change_waiting_time(){
  int eprom_position = 0;
  // Check which program was selected and then load the EEPROM values
  if (selected == 12){
    eprom_position = 0;
  }
  else if (selected == 20){
    eprom_position = 100;
  }
  else if (selected == 28){
    eprom_position = 200;
  }

  // Now read in from EEProm and display values on screen
  CoatingProgram temp;
  EEPROM.get(eprom_position, temp);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting Time ms ");
  lcd.setCursor(0, 1);
  lcd.print(temp.waiting_time);
  
  // Let the user change the value of the waiting time with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
          if (temp.waiting_time < 50000){
            temp.waiting_time += 100;
          }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (temp.waiting_time > 0){
            // If value gets smaller it can reduce its number of digits
            lcd.setCursor(0,1);
            lcd.print(F("                "));
            
            temp.waiting_time -= 100;
          }
      }
      previous_encoder_value = enc.getValue();

      // Display changed value
      lcd.setCursor(0, 1);
      lcd.print(temp.waiting_time);
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
       // Save new value in EEPROM
      EEPROM.put(eprom_position, temp);
       
      // Resume back to menu
      show_menu();  
      break;
     }
    } 
  }
}


void calibrate_motor(){
  // Show initialising on screen
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Initialising... "));

  bool signal_triggered = 0;
  
  digitalWrite(sm_sleep, HIGH);
  stepper1.setMaxSpeed(5000); // Set maximum speed value for the stepper
  stepper1.setAcceleration(5000); // Set acceleration value for the stepper
  stepper1.setCurrentPosition(0);
  
  stepper1.moveTo(100000 * no_microsteps);
  while(stepper1.currentPosition() != 100000 * no_microsteps){
    if (digitalRead(bottom_stop_switch) && not signal_triggered){
      signal_triggered = 1;
      stepper1.stop();
      stepper1.setCurrentPosition(min_position);
      stepper1.moveTo(max_position * no_microsteps);
    }
    if (stepper1.currentPosition() == max_position * no_microsteps)
    {
      break;
    }
    stepper1.run();
  }
  digitalWrite(sm_sleep, LOW);

  //stepper1.moveTo(-45000);

   // Resume back to menu
  show_menu();
}

void move_to_bottom(){  
  safe_move_stepper(-100 * no_microsteps);
}

void move_to_top(){  
  safe_move_stepper(max_position * no_microsteps);
}

void safe_move_stepper(long stepper_position){
  // Release motor from sleep
  stepper_position = stepper_position * no_microsteps;
  digitalWrite(sm_sleep, HIGH);
  
  if (stepper_position <= min_position && stepper_position >= max_position * no_microsteps){
    stepper1.moveTo(stepper_position);
    
    while(stepper1.currentPosition() != stepper_position){
      if (digitalRead(bottom_stop_switch)){
        stepper1.stop();
        stepper1.setCurrentPosition(0);
        
        stepper1.moveTo(max_position * no_microsteps);
        stepper1.runToPosition();
        break;
      }
      stepper1.run();
    }
  }
  
  // Put motor to sleep
  digitalWrite(sm_sleep, LOW);
}

void change_microsteps(){
  // Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Microsteps ");
  lcd.setCursor(0, 1);
  lcd.print(no_microsteps);

  // Index that tells us where we are
  int index;
  
  for (int i=0; i<available_microsteps; i++) {
     if (no_microsteps == available_microsteps[i]) {
       index = i;
       break;
     }
  } 
  
  // Let the user change the value of the bottom position with the knob
  while(1){
    enc.tick();
    // If the previous and the current state of the clock are different, that means a step has occured
    if (enc.valueChanged()){
      if (enc.getValue() < previous_encoder_value){
          if (index < (no_of_different_microsteps -1)){
            index++;       
          }
      }
      else if (enc.getValue() > previous_encoder_value)
      {
          if (index > 0){
            // If value gets smaller it can reduce its number of digits
            index--;
          }
      }
      previous_encoder_value = enc.getValue();

      // Display changed value            
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      lcd.setCursor(0, 1);
      lcd.print(available_microsteps[index]);   
    }
    if (enc.buttonChanged()){
     if (enc.getButton() == LOW){
    no_microsteps = available_microsteps[index];

      // Actually set the values for the pins here with a massive if loop
      if(no_microsteps == 1){
        // Full step
        digitalWrite(sm_m0, LOW);
        digitalWrite(sm_m1, LOW);
        digitalWrite(sm_m2, LOW);
      }
      else if(no_microsteps == 2){
        // Half step
        digitalWrite(sm_m0, HIGH);
        digitalWrite(sm_m1, LOW);
        digitalWrite(sm_m2, LOW);
      }
      else if(no_microsteps == 4){
        // 1/4 step
        digitalWrite(sm_m0, LOW);
        digitalWrite(sm_m1, HIGH);
        digitalWrite(sm_m2, LOW);
      }
      else if(no_microsteps == 8){
        // 1/8 step
        digitalWrite(sm_m0, HIGH);
        digitalWrite(sm_m1, HIGH);
        digitalWrite(sm_m2, LOW);
      }
      else if(no_microsteps == 16){
        // 1/16 step
        digitalWrite(sm_m0, LOW);
        digitalWrite(sm_m1, LOW);
        digitalWrite(sm_m2, HIGH);
      }
      else if(no_microsteps == 32){
        // 1/32 step
        digitalWrite(sm_m0, HIGH);
        digitalWrite(sm_m1, HIGH);
        digitalWrite(sm_m2, HIGH);
      }
       
      // Resume back to menu
      show_menu();  
      break;
     }
    } 
  }
}

void show_menu(void)
{
  unsigned char line_cnt = 0;
  unsigned char from = 0;
  unsigned char till = 0;
  unsigned char temp = 0;

  while (till <= selected)
  {
    till += menu[till].num_menupoints;   

  }   
  from = till - menu[selected].num_menupoints;  
  till--;                     
  temp = from;                
  
  // browsing somewhere in the middle
  if ((selected >= (from+PER_SPACE)) && (selected <= till ))
  {
    from = selected-PER_SPACE;
    till = from + (DISPLAY_ROWS-1);
    
    for (from; from<=till; from++)
    {
      lcd.setCursor(0, line_cnt);
      lcd.print(menu[from].text);
      line_cnt = line_cnt + 1;
    }
  }
  
  // browsing somewhere in the top or the bottom
  else
  {
    // top of the menu
    if (selected < (from+PER_SPACE))  // 2 lines
    {
      //till = from + 3;
      till = from + (DISPLAY_ROWS-1); // 2 lines

      for (from; from<=till; from++)
      {
        lcd.setCursor(0, line_cnt);
        lcd.print(menu[from].text);
        line_cnt = line_cnt + 1;
      }
    }

    // bottom of the menu
    if (selected == till)
    {
      from = till - (DISPLAY_ROWS-1); // 2 lines
      
      for (from; from<=till; from++)
      {
        lcd.setCursor(0, line_cnt);
        lcd.print(menu[from].text);
        line_cnt = line_cnt + 1;     
      }
     
    }
  }
  lcd.setCursor(0, 2);
  lcd.print(">");
}
