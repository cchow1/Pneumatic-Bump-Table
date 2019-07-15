#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>


/*
//This code is made by Raymond Lee (Test Engineer Co-op) November 2018
//Project: Pneumatic Bump Table
//Note: "A" is defined to be "Down" movement of the cylinder 
//      "B" is defined to be "Up" movement of the cylinder 
//The pneumatic cylinder is a double action solenoid valve driven cylinder      
//Two solenoid valve (A & B) has Three Actions: Up, Down, No Movement
//     *  A = HIGH & B = HIGH *  == No movement
//     *  A = LOW  & B = LOW *   == No movement
//     *  A = LOW  & B = HIGH *  == UP
//     *  A = HIGH & B = LOW *   == DOWN
//
//EEPROM: each address can only store 1 byte which is 255, 
//        allowable store goal value from 0 to 2550
//                     counter value from 0 to 2550 
//        allowable counter value in increment of 10 (ex: 126 cycles will store 120 in eeprom)
//
//Download LCD_I2C library: https://github.com/marcoschwartz/LiquidCrystal_I2C 
*/

/*Button setup*/
char msgs[5][15] = {
 "UP Key OK ",
 "Left Key OK ",
 "Down Key OK ",
 "Right Key OK ",
 "Select Key OK" };
char start_msg[15] = {
 "Start loop "};
int adc_key_val[5] ={
 30, 150, 360, 535, 760 };
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;


/*EEPROM Setup*/
int eeprom_goal_adr = 1;
int eeprom_counter_adr = 2;
byte value;

/*Soleniod Valve Setup*/
const int up = 7;    // set pin 7 (B) digital to be the pin for triggering UP movement 
const int down = 5;  // set pin 5 (A) digital to be the pin for triggering DOWN movement 


/*Counter setup*/
int goal_set = 10;
int counter_set = 0;
int stored_counter_set = 5;
int stored_goal_set = 100;
int page=1;  
char code_complete = false;
char pause_trigger = false;
int goal_error;
int oldpage=0;
int delay_up = 1000;
int delay_down = 1000;
int g=0;
int h=0;
int pre_count_set;
int skip_page_5 = true;
int initial_count = 0;

// constants won't change. Used here to set a pin number:
const int ledPin =  LED_BUILTIN; // the LED pin number 

/*LCD I2C setup*/
#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

//play around with custom characters on LCD screen
uint8_t bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t note[8]  = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
uint8_t heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};
uint8_t duck[8]  = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0};
uint8_t retarrow[8] = {  0x1,0x1,0x5,0x9,0x1f,0x8,0x4};
  
LiquidCrystal_I2C lcd(0x20,20,4);  // set the LCD address to 0x20 for a 20 chars and 4 line display (All jumpers should be connected!)

/*Proximity sensor setup*/
const int InfraredSensorPin = 10;   //Connect the sensor signal pin to the digital pin 10
int Sensoroff = false;      //Turn off sensor function when true;



void setup() {
 //initialize what each pin does (input or output)
       pinMode(ledPin, OUTPUT); 
       pinMode(up, OUTPUT); 
       pinMode(down, OUTPUT); 
       pinMode(InfraredSensorPin,INPUT);
       
//       delay(3000);
       int f = 0;

        lcd.init();                      // initialize the lcd 
        lcd.backlight();
        
        lcd.createChar(0, bell);
        lcd.createChar(1, note);
        lcd.createChar(2, clock);
        lcd.createChar(3, heart);
        lcd.createChar(4, duck);
        lcd.createChar(5, check);
        lcd.createChar(6, cross);
        lcd.createChar(7, retarrow);
        lcd.home();
       
       Serial.begin(9600);
       /* Print Welcome page here */
       Serial.println("Hello HOLA SALAAM"); // Page 1 welcome page
         lcd.setCursor(5,0);
         lcd.print("Bump Table");
       
       read_eeprom(); //call a function
       
       Serial.print("EEPROM Goal:");
       Serial.println(stored_goal_set); 
       Serial.print("EEPROM Counter:");
       Serial.println(stored_counter_set); 
         lcd.setCursor(0,2);
         lcd.print("EEPROM Goal:");  
         lcd.setCursor(14, 2);       
         lcd.print(stored_goal_set);
         lcd.setCursor(0, 3);   
         lcd.print("EEPROM Counter: ");  
         lcd.setCursor(16, 3);       
         lcd.print(stored_counter_set);

      delay(3000);
      lcd.clear();
         
      
 }

/*Convert number to store in EEPROM*/
int store_eeprom(int value, int input)
   {
      int a = value/256;
      int b = value % 256;
      if(input == 1){
          EEPROM.update(0,a);
          EEPROM.update(1,b);
        }else if (input == 2){
          EEPROM.update(10,a);
          EEPROM.update(11,b);
          }
     
   }

/*read EEPROM storage number*/
int read_eeprom()
   {  
    for(int i=0;i<2;i++){
    if(i==0){    
      int a=EEPROM.read(0);
      int b=EEPROM.read(1);
      stored_goal_set = a*256+b;
    
    }else if(i==1){
      int a=EEPROM.read(10);
      int b=EEPROM.read(11);
      stored_counter_set = a*256+b;
      }
    }  
  }


/* Convert ADC value to key number S1=0, S2=144, S3=333, S4=742, No press=1023 */
int get_key(unsigned int input)
    {
       int k;
       for (k = 0; k < NUM_KEYS; k++)
       {
       if (input < adc_key_val[k])
       {
       return k;
       }
       }
       if (k >= NUM_KEYS)
       k = -1; // No valid key pressed
       return k;
    } 


/*Check Button on the Arduino Board*/
void button_press(){

      int key_press_temp = 0;
      int hold = 6;
      
     adc_key_in = analogRead(0); // read the value from the sensor
     digitalWrite(13, HIGH);
     
     /* get the key */
     key_press_temp = get_key(adc_key_in); // convert into key press
 
     if (key_press_temp != -1) {           // if keypress is detected; (-1 value is the no button pressed state)
        
         delay(50); // wait for debounce time
         adc_key_in = analogRead(0); // read the value from the sensor
         while(hold >= 0){   //This while loop waits for the user to release the button 
             adc_key_in = analogRead(0);   // read the value from the sensor
             hold = get_key(adc_key_in);   
           }
        
         key = key_press_temp;   //send key value back to global variable "key"
         
         if (key != oldkey) {          
             oldkey = key;
          }
     }
     digitalWrite(13, LOW);
    
}


int manual_auto_select()
   {
         int cmd;
         char next_step = false;
         
         Serial.println("--------------------"); // Divider line 
         Serial.println("   Operation Mode   ");         
         Serial.println("  Manual      Auto  ");     //button selection
           lcd.setCursor(0,0);
           lcd.print("   Operation Mode   ");  
           lcd.setCursor(0, 3);   
           lcd.print(" Manual        Auto ");  

         
         delay(500);
         while(analogRead(0)>1000) { };   //wait for the button to be pressed
         
         while(next_step == false){
            
            button_press();
            //delay(1000);
            cmd = key;
                       
            if(cmd == 1){
                goal_set=0;
                next_step=true;
                page = 8;
              }else if(cmd == 3){              
                 next_step=true;
                 page = 2;
                }
         }
         lcd.clear();
         delay(50);
   }



int previous_goal_set()
   {
         int cmd;
         char next_step = false; 

         Serial.println("--------------------"); // Divider line 
         Serial.print("Previous Goal: "); 
           lcd.setCursor(0, 0);   
           lcd.print("Previous Goal: ");  
          if(g==0){
                  Serial.println(stored_goal_set);
                     lcd.setCursor(15, 0);   
                     lcd.print(stored_goal_set); 
                  goal_set = stored_goal_set;}
               else {
                  Serial.println(goal_set); 
                    lcd.setCursor(15, 0);   
                    lcd.print(goal_set);}    
         Serial.println("Keep these value?");
           lcd.setCursor(0, 1);   
           lcd.print("Keep these value?");
         Serial.println(" NO              YES");     //button selection
           lcd.setCursor(0, 3);   
           lcd.print(" NO             YES ");
              
         delay(500);
         while(analogRead(0)>1000) { };
         
         while(next_step == false){
            button_press();
            //delay(1000);
            cmd=key;
           
            if(cmd==1){
                  goal_set=0;
                  next_step=true;
                  page = 3;
              }else if(cmd==3){
                 next_step=true;
                 page = 4;
                }
         }  
         lcd.clear();
         delay(50);
   }


int current_goal_set()
   {
         int cmd;
         char next_step = false; 

         while(next_step == false){ 
           
            Serial.println("--------------------"); // Divider line 
            Serial.println("        SET         "); 
              lcd.setCursor(0, 0);   
              lcd.print("        SET         ");        
            Serial.print("Goal: ");
            Serial.println(goal_set); 
              lcd.setCursor(0, 1);   
              lcd.print("Goal: "); 
              lcd.setCursor(6, 1);   
              lcd.print(goal_set); 
            Serial.println("  -      OK      +  ");    //button selection
              lcd.setCursor(0, 3);   
              lcd.print("  -      OK      +  "); 


            
            delay(100);
            while(analogRead(0)>1000) { };
            
            button_press();
            //delay(1000);
            cmd=key;
            
            if(cmd==1 && goal_set!=0){
              goal_set-=10;
              }else if(cmd==2){ 
                 next_step=true;
                 page=4;              
                } else if(cmd==3){
                    goal_set+=10;
                   } 
              
         }    
         lcd.clear();
         delay(50);                         
   }


int previous_count_set()
   {
         int cmd;
         char next_step = false; 

         Serial.println("--------------------"); // Divider line 
         if(goal_error == true){
            Serial.println("Goal <= completed "); 
               lcd.setCursor(0, 0);   
               lcd.print("Goal <= completed ");
            }
         Serial.print("Prev. Complete: ");         
         Serial.println(stored_counter_set); 
            lcd.setCursor(0, 1);   
            lcd.print("Prev Complete:");
            lcd.setCursor(15, 1);   
            lcd.print(stored_counter_set);
         Serial.println("RESET   BACK    KEEP");   //button selection
            lcd.setCursor(0, 3);   
            lcd.print("RESET   BACK    KEEP");
         
         delay(500);
         while(analogRead(0)>1000) { };     //wait for the button to be pressed

         while(next_step == false){
           
            button_press();
            //delay(1000);
            cmd=key;
            
            if(cmd==1){
                counter_set = 0;
                next_step=true;
                page =5;
                h=false;
                initial_count= false;
                if(skip_page_5 == true){
                  page = 6;
                  }
                goal_error = false;  
              }else if(cmd==2){                
                 next_step=true;
                 page=2;   
                 goal_error = false;         
                 } else if(cmd==3){
                     
                     counter_set = stored_counter_set;
                     next_step=true;
                     page =5;
                     initial_count= true;
                     h=true;
                     if(skip_page_5 == true){
                         page = 6;
                         }
                     goal_error = false;
                     if(goal_set <= stored_counter_set){
                          page = 4;
                          goal_error = true;
                      }
                   } 
             
         }                         
        lcd.clear();
        delay(50);
   }


   int current_count_set()
   {
         int cmd;
         char next_step = false; 
         int new_count = pre_count_set;
         
         Serial.println("--------------------"); // Divider line 
         if(goal_error == 1){
            Serial.println("Goal <= Count "); 
               lcd.setCursor(0, 0);   
               lcd.print("Goal <= Count");
            }
         Serial.print("Newly Complete: ");         
         Serial.println(new_count); 
            lcd.setCursor(0, 1);   
            lcd.print("Newly Complte: ");
            lcd.setCursor(15, 1);   
            lcd.print(new_count);
         Serial.println("RESET   BACK    KEEP");   //button selection
            lcd.setCursor(0, 3);   
            lcd.print("RESET   BACK    KEEP");
         
         delay(500);
         while(analogRead(0)>1000) { };     //wait for the button to be pressed
         
         while(next_step == false){
           
            button_press();
            //delay(1000);
            cmd=key;


            switch(cmd){
              
              case 1:
                  next_step=true;
                page =6;
                goal_error = false;
                if(h==true){
                    counter_set=stored_counter_set;
                    h=false;
                  }else{counter_set= 0;} 
                  break;
              
              case 2:
                   next_step=true;
                   page=2;   
                   goal_error = false; 
                   break;

              case 3:
               next_step=true;
                     page =6;
                     goal_error = false;
                     if(h==true){
                         counter_set= new_count+stored_counter_set;
                         h=false;
                         if(goal_set <= counter_set){
                          page = 5;
                          goal_error = true;
                          h=true;}
                     }else if(h==false){
                        counter_set= new_count;
                        if(goal_set <= counter_set){
                          page = 5;
                          goal_error = true;
                          h=false;}
                      }
                      break;
              
              
              }
             
         }                         
        lcd.clear();
        delay(50);
   }

/*set delay time for both UP and DOWN movment*/
int delay_time_select()
  {
         int cmd;
         char next_step = false; 
         int number=0;
         int delay_temp=500;
         int i=0;
         
         Serial.println("--------------------"); // Divider line 
         Serial.println("        SET         ");    
            lcd.setCursor(0, 0);   
            lcd.print("        SET         "); 
            lcd.setCursor(0, 3);   
            lcd.print("  -      OK      + ");  
        
         
         while(next_step == false){ 
           
            Serial.println("--------------------"); // Divider line      
            if(i==0){
                  Serial.print("UP Delay Time:");
                     lcd.setCursor(0, 1);   
                     lcd.print("UP Delay Time:");}             
               else {
                  Serial.print("DOWN Delay Time:");
                     lcd.setCursor(0, 1);   
                     lcd.print("DWN Delay Time:"); }   
            Serial.println(delay_temp);
                lcd.setCursor(16, 1);   
                lcd.print(delay_temp);
            Serial.println("  -      OK      + ");    //button selection

            while(analogRead(0)>1000) { };
            
            button_press();
            //delay(1000);
            cmd=key;

            lcd.setCursor(16, 1);   
            lcd.print("    ");
            delay(100);
                        
            if(cmd==1 && delay_temp !=0){
              delay_temp-=100;
              }
              else if(cmd==2 && i==1 && delay_temp !=0){ 
                 next_step=true;
                 delay_down = delay_temp;
                 page=7; 
                   } else if(cmd==2 && delay_temp !=0 && i!=1 ){  
                         page=6;   
                         delay_up = delay_temp;
                         i=1; }        
                else if(cmd==3){
                    delay_temp+=100;
                   }              
         }   
         lcd.clear();
         delay(50);            
  }


int start_check()
   {
         int cmd;
         char next_step = false; 
         

         Serial.println("------------------"); // Divider line 
         Serial.print("Goal: ");         
         Serial.println(goal_set);
            lcd.setCursor(0, 0);   
            lcd.print("Goal: "); 
            lcd.setCursor(9, 0);   
            lcd.print(goal_set);
         Serial.print("Counter: ");         
         Serial.println(counter_set); 
            lcd.setCursor(0, 1);   
            lcd.print("Counter: "); 
            lcd.setCursor(9, 1);   
            lcd.print(counter_set); 
         Serial.println("Set   Manual   Start");    //button selection
            lcd.setCursor(0, 3);   
            lcd.print("Set   Manual   Start");  
         
         
         delay(100);
         
         
         while(next_step == false){
          
            Serial.print("Sensor: ");         
            Serial.println(Sensoroff);
                lcd.setCursor(0, 2);   
                lcd.print("Sensor: "); 
                lcd.setCursor(9, 2);   
                  if(Sensoroff == true){lcd.print("OFF");}
                  else{lcd.print("ON");}  

            while(analogRead(0)>1000) { };   //wait for the button to be pressed
              
              button_press();
              //delay(1000);
              cmd=key;
              
              if(cmd==1){
                next_step=true;
                page = 2;
                }else if(cmd==2){
                   next_step=true;
                   oldpage=1;
                   page = 8;
                  } else if(cmd==3){
                     next_step=true;
                     page = 9; 
                     pause_trigger=false; 
                     }else if(cmd==4){
                       next_step=true;
                       page = 7;
                       Sensoroff = !Sensoroff; 
                       }
         } 
        lcd.clear();
        delay(50);
   }


int manual_op()
   {
         int cmd;
         char next_step = false; 
        
         Serial.println("--------------------"); // Divider line 
         Serial.println("DOWN     BACK     UP");       //button selection   
            lcd.setCursor(0, 3);   
            lcd.print("DOWN    BACK     UP");

         while(next_step == false){
     
               delay(500);
               while(analogRead(0)>1000) { };  //wait for the button to be pressed
               
              
                  button_press();
                  //delay(1000);
                  cmd=key;
                   
                  if(cmd==1){                         //Down
                    digitalWrite(down, LOW);
                    digitalWrite(up, LOW);
                    digitalWrite(down, HIGH);
                    }else if(cmd==2){
                           next_step=true;
                           if(oldpage==1){
                                page=7; //back to the original page that the user came from 
                                oldpage=0;
                           }   
                               else{page=1;}       
                        }else if(cmd==3){               //Up
                             digitalWrite(up, LOW);
                             digitalWrite(down, LOW);
                             digitalWrite(up, HIGH);
                         
                      } 
                
         } 
                                
        lcd.clear();
        delay(50);
   }

int pause_check(){   //check if PAUSE button was triggered, if YES then pause action
      int cmd;
      button_press();
      delay(200);
      cmd=key;
                
      if(cmd==2){
          digitalWrite(up, LOW);        // LOW & LOW == no movement
          digitalWrite(down, LOW);
          page = 7;
          pause_trigger=true;
          if(initial_count == true){
            pre_count_set = counter_set-stored_counter_set;
            initial_count= false;
            }else {pre_count_set = counter_set;}
          return 1;
          } 
      return 0;
}



int auto_op()
   {
    
    char next_step = false; 
    int user_pause = false;
    int move_count = goal_set-counter_set; 
    int h=1;

    skip_page_5 = false;
    g = 1; 
    store_eeprom(goal_set, 1); // stored the pre-determined goal target to EEPROM

    digitalWrite(up, LOW);         //Set UP&DOWN to be LOW to pause the movement of the cylinder
    digitalWrite(down, LOW); 
    digitalWrite(up, HIGH);        //Set first movement to be UP 
    delay(delay_up);

      
        lcd.setCursor(0, 0);   
        lcd.print("Goal: "); 
        lcd.setCursor(9, 0);   
        lcd.print(goal_set);
        lcd.setCursor(0, 1);   
        lcd.print("Counter: "); 
        lcd.setCursor(0, 3);   
        lcd.print("       PAUSE      ");  
        lcd.setCursor(0, 2);   
        lcd.print("Sensor: "); 
        lcd.setCursor(9, 2);   
           if(Sensoroff == true){lcd.print("OFF");}
           else{lcd.print("ON");}

   
           for(int i = 1  ; i < move_count; i++ ){      //move cylinder    
            
                      user_pause = pause_check();       //Check if pause button is pressed
                      if(user_pause==1){                //If pressed, exit the loop
                         break;}
                        
                      digitalWrite(up, LOW);            //move cylinder DOWN
                      digitalWrite(down, HIGH);
                      counter_set +=1;      //incease counter by 1

                     if(Sensoroff == false){                               //if sensor is turned on, then enter 
                          while(digitalRead(InfraredSensorPin) == LOW)     //wait for platform to raise to down position
                          { 
                            user_pause = pause_check();       //Check if pause button is pressed
                                if(user_pause==1){                //If pressed, exit the loop
                                   break;}
                            digitalWrite(ledPin,HIGH);
                            if(h==1){Serial.print("Down Switch Status:");
                            Serial.println(digitalRead(InfraredSensorPin),BIN);
                            h=0;}
                          } 
                            Serial.print("Down Status change to:");
                            Serial.println(digitalRead(InfraredSensorPin),BIN);
                            digitalWrite(ledPin,LOW);
                            h=1;
                     }    
                     
                      Serial.println("------------------"); // Divider line 
                      Serial.print("Goal: ");         
                      Serial.println(goal_set);
                      Serial.print("Counter: ");         
                      Serial.println(counter_set);
                        lcd.setCursor(9, 1);   
                        lcd.print(counter_set);
                      Serial.print("Sensor: ");         
                      Serial.println(Sensoroff); 
                      Serial.println("       PAUSE      ");     //button selection
                     
                      delay(delay_down);
                     
                      user_pause = pause_check();       //Check if pause button is pressed
                      if(user_pause==1){                //If pressed, exit the loop
                         break;}
                        
                      digitalWrite(down, LOW);          //move cylinder UP
                      digitalWrite(up, HIGH);
                     

                      if(Sensoroff == false){
                          while(digitalRead(InfraredSensorPin) == HIGH)     //wait for platform to raise to top position
                          { 
                            user_pause = pause_check();       //Check if pause button is pressed
                                if(user_pause==1){                //If pressed, exit the loop
                                   break;}
                            digitalWrite(ledPin,HIGH);
                            if (h==1){Serial.print("Up Switch Status:");
                            Serial.println(digitalRead(InfraredSensorPin),BIN);
                            h=0;}
                          }
                            Serial.print("Up Status change to:");
                            Serial.println(digitalRead(InfraredSensorPin),BIN);
                            digitalWrite(ledPin,LOW);
                            h=1;
                      }

                      delay(delay_up);
                      
                     if (counter_set % 10 == 0){              //update the EEPROM counter value every 10 cycle into address 
                          store_eeprom(counter_set,2);
                      }  
                     lcd.setCursor(9, 1);             //print space to overwrite the counter value for be updated
                     lcd.print("       ");
                     
           } 
            
           
    
     if(pause_trigger == false){        //This if statment checks the state of exiting the page;  
                                        //If pause button is pressed, then this will not be executed
         digitalWrite(up, LOW);         //Bring cylinder DOWN for the last move and stay down
         digitalWrite(down, HIGH);
         delay(delay_down); 
         digitalWrite(down, LOW);
         Serial.println("      Last Move     ");  
             lcd.setCursor(0, 2);   
             lcd.print("      Last Move     ");  
         counter_set = goal_set;
         stored_counter_set = counter_set;
         store_eeprom(goal_set,2);       //update EEPROM counter value to be the same as goal value        
         page = 10;
         if(initial_count ==1){
            pre_count_set = counter_set-stored_counter_set;
            initial_count=0;
            }else {pre_count_set = counter_set;}
     } 
     lcd.clear();
     delay(50);
}

int finish()
  {
      int cmd;
      
      Serial.println("--------------------"); // Divider line 
      Serial.println("    Test Complete   ");
          lcd.setCursor(0, 1);   
          lcd.print("    Test Complete   "); 
      Serial.println("      Restart       ");     //button selection
          lcd.setCursor(0, 3);   
          lcd.print("      Restart       "); 
      
      while(analogRead(0)>1000) { };  //wait for the button to be pressed
             
            
         button_press();
         //delay(1000);
          cmd=key;
                
         if(cmd==2){
              page = 1;     
           }  

        lcd.clear();
        delay(50);
  }


void loop()
{
 
    while(code_complete == false){
       
        switch(page){
          
             case 1: 
                manual_auto_select();
                break;
             
             case 2:
                previous_goal_set();  
                break; 
      
            case 3:
                current_goal_set();
                break;  
            
            case 4:
                previous_count_set();   
                break; 
                
            case 5:
                current_count_set();   
                break; 
                
            case 6:
                delay_time_select();
                break;    
            
            case 7:
                start_check();
                break; 
                
            case 8:
                manual_op();
                break; 
                
            case 9:
                auto_op();
                break; 
            
            case 10: 
                finish();
                break;      
          
         }        
    }
}
 
