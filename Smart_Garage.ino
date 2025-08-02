// Display system
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Fire system
#define Flame_sensor A1
#define Buzzer 7
#define Red_led 8

// Light system
#define White_led 6
#define LDR A0 

// Gate system
#define sensor1 3 
#define sensor2 4
#include<Servo.h>
Servo angle;     //Gate Servo

//Parking System
#define Parking_sensor 2 
Servo slots_angle;  //Parking Gate

#define Slot_1 0  //Parking Slots
#define Slot_2 1
#define Slot_3 2

#include <SoftwareSerial.h>  //Bluetooth
SoftwareSerial bluetooth(10, 11); // Changed from pins 0,1 to avoid conflict with hardware serial

int Parking_sensor_reading=1;
int current_slot;
int primary_current_slot=0;
int Slots[3] = {0, 0, 0};

int sensor1_reading=1; //if there is a partical for sensor1
int sensor2_reading=1; //if there is a partical for sensor2
int Free_space=3; // number of parking spaces
int wait_for_sensor1=0; // when we open gate 
int wait_for_sensor2=0;
int stop=0;
int open_time=0;
int calc_time=0;

// Debouncing variables
unsigned long lastSensor1Time = 0;
unsigned long lastSensor2Time = 0;
unsigned long lastParkingTime = 0;
const unsigned long debounceDelay = 50; // 50ms debounce

// LCD update variables
int lastFreeSpace = -1;
int lastSlots[3] = {-1, -1, -1};
unsigned long lastLCDUpdate = 0;
const unsigned long lcdUpdateInterval = 500; // Update LCD every 500ms

// Fire system variables
bool fireDetected = false;
unsigned long fireBlinkTime = 0;
bool fireBlinkState = false;

void setup()
{
  angle.attach(5);
  pinMode(sensor1,INPUT);
  pinMode(sensor2,INPUT);
  angle.write(0);

  pinMode(Buzzer,OUTPUT);
  pinMode(Flame_sensor,INPUT);
  pinMode(Red_led,OUTPUT);

  pinMode(LDR, INPUT);
  pinMode(White_led, OUTPUT);

  slots_angle.attach(9);
  pinMode(Parking_sensor,INPUT);
  slots_angle.write(0);
  current_slot=1;

  lcd.begin(16, 2);   // initialize
  lcd.clear();
  lcd.setCursor(0, 0);
  Serial.begin(9600);

  bluetooth.begin (9600);
  delay(1500);
}

// Debounced digital read function
bool debouncedDigitalRead(int pin, unsigned long &lastTime) {
  bool reading = digitalRead(pin);
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime > debounceDelay) {
    lastTime = currentTime;
    return reading;
  }
  return HIGH; // Return HIGH (no detection) if within debounce period
}

// Efficient LCD update function
void updateLCDDisplay() {
  unsigned long currentTime = millis();
  
  // Only update LCD if enough time has passed or values have changed
  if (currentTime - lastLCDUpdate >= lcdUpdateInterval || 
      Free_space != lastFreeSpace || 
      Slots[0] != lastSlots[0] || 
      Slots[1] != lastSlots[1] || 
      Slots[2] != lastSlots[2]) {
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Empty Slots:");
    lcd.setCursor(13,0);
    lcd.print(Free_space);
    lcd.setCursor(0,1);
    lcd.print(" S1:");
    lcd.print(Slots[0]);
    lcd.print(" S2:");
    lcd.print(Slots[1]);
    lcd.print(" S3:");
    lcd.print(Slots[2]);
    
    // Update last values
    lastFreeSpace = Free_space;
    lastSlots[0] = Slots[0];
    lastSlots[1] = Slots[1];
    lastSlots[2] = Slots[2];
    lastLCDUpdate = currentTime;
  }
}

void loop()
{

//====================================================================
//                         Display System
//====================================================================

  updateLCDDisplay();

//====================================================================
//                         Gate System
//====================================================================

 sensor1_reading = debouncedDigitalRead(sensor1, lastSensor1Time);
 sensor2_reading = debouncedDigitalRead(sensor2, lastSensor2Time);


  if(sensor1_reading==0 && stop==0 && Free_space > 0)
  { 
    for(int y=0;y<=90;y=y+5)
      {  
      angle.write(y);
      delay(100);
      wait_for_sensor2=1;
      }
     stop=1;
     open_time= millis();
    Serial.print(open_time);
  }

  if(sensor2_reading==0 && stop==0 && Free_space < 3)
  { 
    for(int y=0;y<=90;y=y+5)
    {  
      angle.write(y);
      delay(100);
      wait_for_sensor1=1;
    }
    stop=1;
    open_time= millis();
    Serial.print(open_time);
  }

  calc_time= millis();

  if(wait_for_sensor2==1)
  {
    if(sensor2_reading==0)
    {
      Free_space--;
      wait_for_sensor2=0;
      stop=0;

      for(int y=90;y>=0;y=y-5)
      {
      angle.write(y); 
      delay(100);
      } 
      delay(700);
      open_time=0;
    }
  }

    if(wait_for_sensor1==1)
  {
    if(sensor1_reading==0)
    {
      Free_space++;
      wait_for_sensor1=0;
      stop=0;
      for(int y=90;y>=0;y=y-5)
      {
      angle.write(y); 
      delay(100);
      } 
      delay(700);
      open_time=0;
    }
  }

    if(open_time > 0 && calc_time >= open_time + 3000 ){
      wait_for_sensor1=0;
      wait_for_sensor2=0;
      stop=0;
      for(int y=90;y>=0;y=y-5)
      {
      angle.write(y); 
      delay(100);
      } 
      Serial.print(" ");
      Serial.println(calc_time);
      open_time=0; // Reset to prevent repeated execution
    }

  if(Free_space > 3){
    Free_space=3;
  }
  else if(Free_space <0){
    Free_space=0;
  }


  
//====================================================================
//                         Lightning System
//====================================================================  

  int LDR_Reading = analogRead(LDR);
  int White_led_value = map(LDR_Reading, 0, 1023, 0, 255);
  if(White_led_value > 150){
  analogWrite(White_led, 255);
  }
  else{
  analogWrite(White_led, 0);
  }

//====================================================================
//                         Parking System
//====================================================================

  Parking_sensor_reading = debouncedDigitalRead(Parking_sensor, lastParkingTime);

  // Auto Slot Changer (In Case the current slot is used)

  if(Parking_sensor_reading==0 && current_slot==3)
  { 
    delay(1000);
    Slots[Slot_3]=1;
  }

  if(Parking_sensor_reading==0 && current_slot==2)
  { 
    delay(1000);
    for(int y=90;y<=180;y=y+5)
      {  
      slots_angle.write(y);
      delay(100);
      }
    current_slot=3;
    Slots[Slot_2]=1;
  }

  if(Parking_sensor_reading==0 && current_slot==1)
  { 
    delay(1000);
    for(int y=0;y<=90;y=y+5)
      {  
      slots_angle.write(y);
      delay(100);
      }
    current_slot=2;
    Slots[Slot_1]=1;
  }

  // Blutooth controller 

  // if(bluetooth.available())
  // {
  //   char selected_slot = bluetooth.read();

  //   //////////////////////////////////////////////////

  //   if(selected_slot == '1') // Go to slot 1
  //   {

  //     if(current_slot==3){
  //         for(int y=180; y>=0; y=y-5)
  //         {  
  //         slots_angle.write(y);
  //         delay(100);
  //         }        
  //     }

  //     else if(current_slot==2){
  //         for(int y=90; y>=0; y=y-5)
  //         {  
  //         slots_angle.write(y);
  //         delay(100);
  //         }
  //     }
      
  //     else if(current_slot==1){
  //       slots_angle.write(0);
  //     }
  //     primary_current_slot=1;
  //     selected_slot = bluetooth.read();
  //   }

  //   //////////////////////////////////////////////////
    
  //   else if(selected_slot == '2') // Go to slot 2
  //   {

  //     if(current_slot==3){
  //       for(int y=180; y>=90; y=y-5)
  //       {  
  //       slots_angle.write(y);
  //       delay(100);
  //       }
  //     }

  //     else if(current_slot==2){
  //       slots_angle.write(90);
  //     }

  //     else if(current_slot==1){
  //       for(int y=0;y<=90;y=y+5)
  //       {  
  //       slots_angle.write(y);
  //       delay(100);
  //       }
  //     }
  //     primary_current_slot=2;
  //     selected_slot = bluetooth.read();
  //   }
    
  //   //////////////////////////////////////////////////
    
  //   else if(selected_slot == '3') // Go to slot 3
  //   {
      
  //     if(current_slot==3){
  //       slots_angle.write(180);
  //     }

  //     else if(current_slot==2){
  //       for(int y=90;y<=180;y=y+5)
  //       {  
  //       slots_angle.write(y);
  //       delay(100);
  //       }
  //     }

  //     else if(current_slot==1){
  //       for(int y=0;y<=180;y=y+5)
  //       {  
  //       slots_angle.write(y);
  //       delay(100);
  //       }
  //     }
  //     primary_current_slot=3;
  //     selected_slot = bluetooth.read();
  //   }

  //     //////////////////////////////////////////////////
    
  //   if (selected_slot == '4') // Enter (the car already left the slot)
  //   {
  //     if (primary_current_slot==1 && Parking_sensor_reading==1){
  //       current_slot=1;
  //       Slots[Slot_1]=0;
  //     }
  //     else if (primary_current_slot==2 && Parking_sensor_reading==1){
  //       current_slot=2;
  //       Slots[Slot_2]=0;
  //     }
  //     else if (primary_current_slot==3 && Parking_sensor_reading==1){
  //       current_slot=3;
  //       Slots[Slot_3]=0;
  //     }
  //     selected_slot = bluetooth.read();
  //   }
  // }

  if(Serial.available())
  {
    char selected_slot = Serial.read();

    //////////////////////////////////////////////////

    if(selected_slot == '1') // Go to slot 1
    {

      if(current_slot==3){
          for(int y=180; y>=0; y=y-5)
          {  
          slots_angle.write(y);
          delay(100);
          }        
      }

      else if(current_slot==2){
          for(int y=90; y>=0; y=y-5)
          {  
          slots_angle.write(y);
          delay(100);
          }
      }
      
      else if(current_slot==1){
        slots_angle.write(0);
      }
      delay(500);
      primary_current_slot=1;
      delay(500);
      selected_slot = Serial.read();
    }

    //////////////////////////////////////////////////
    
    else if(selected_slot == '2') // Go to slot 2
    {

      if(current_slot==3){
        for(int y=180; y>=90; y=y-5)
        {  
        slots_angle.write(y);
        delay(100);
        }
      }

      else if(current_slot==2){
        slots_angle.write(90);
      }

      else if(current_slot==1){
        for(int y=0;y<=90;y=y+5)
        {  
        slots_angle.write(y);
        delay(100);
        }
      }
      delay(500);
      primary_current_slot=2;
      delay(500);
      selected_slot = Serial.read();
    }
    
    //////////////////////////////////////////////////
    
    else if(selected_slot == '3') // Go to slot 3
    {
      
      if(current_slot==3){
        slots_angle.write(180);
      }

      else if(current_slot==2){
        for(int y=90;y<=180;y=y+5)
        {  
        slots_angle.write(y);
        delay(100);
        }
      }

      else if(current_slot==1){
        for(int y=0;y<=180;y=y+5)
        {  
        slots_angle.write(y);
        delay(100);
        }
      }
      delay(500);
      primary_current_slot=3;
      delay(500);
      selected_slot = Serial.read();
    }

      //////////////////////////////////////////////////
    
    if (selected_slot == 'E' || selected_slot == 'e') // Enter (the car already left the slot)
    {
      if (primary_current_slot==1 && Parking_sensor_reading==1){
        current_slot=1;
        Slots[Slot_1]=0;
      }
      else if (primary_current_slot==2 && Parking_sensor_reading==1){
        current_slot=2;
        Slots[Slot_2]=0;
      }
      else if (primary_current_slot==3 && Parking_sensor_reading==1){
        current_slot=3;
        Slots[Slot_3]=0;
      }
      delay(500);
      selected_slot = Serial.read();
    }
  }

//====================================================================
//                         Fire System
//====================================================================

  int Flame_sensor_reading = analogRead(Flame_sensor);

  if(Flame_sensor_reading < 200)
  {
    if (!fireDetected) {
      // Fire just detected
      fireDetected = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Fire alert");
      digitalWrite(Buzzer,HIGH);
      
      // Open gate gradually (non-blocking)
      for(int y=0;y<=90;y=y+5)
      {  
        angle.write(y);
        delay(100);
      }
      fireBlinkTime = millis();
      fireBlinkState = true;
    }
    
    // Non-blocking fire alert blinking
    if (millis() - fireBlinkTime >= 500) {
      fireBlinkState = !fireBlinkState;
      digitalWrite(Red_led, fireBlinkState ? HIGH : LOW);
      lcd.setCursor(11,0);
      lcd.print(fireBlinkState ? "!" : " ");
      fireBlinkTime = millis();
    }
    
    digitalWrite(Buzzer,HIGH);
    angle.write(90);
  }
  else if (fireDetected && Flame_sensor_reading >= 200)
  {
    // Fire cleared
    fireDetected = false;
    digitalWrite(Buzzer,LOW);
    digitalWrite(Red_led,LOW);
    
    // Close gate gradually
    for(int y=90;y>=0;y=y-5)
    {
      angle.write(y); 
      delay(100);
    } 
    lcd.clear();
  }
}