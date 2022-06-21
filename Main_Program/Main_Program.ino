#include <max6675.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define soPin  50
#define csPin  53
#define sckPin 52
#define ROWS    4
#define COLS    4
#define Sel1    2 //selenoid1
#define Sel2    8 //selenoid2 (Api besar)
#define Blow    9 //blower
#define Ig      3 //igniter
#define buzz    4
#define sens1  A1
#define sens2  A2

MAX6675 thermocp(sckPin, csPin, soPin);
LiquidCrystal_I2C lcd(0x27, 16, 2);

struct Time{
  unsigned long currentTime;
  unsigned long lastTime;
  int value;
};
Time Flame1, Flame2, Start, Blower, Burner;

struct Variable{
  int tempU;
  int tempD;
  int adc;
  
  int rtempU;
  int rtempD;
  int radc;
  int rtimer;

  unsigned long timer;
};
Variable Single, Double;

enum Tahap{start, process, temperatur, buzzer, burner};
Tahap StepSingle, StepDouble;

enum Numb{mode, cmode, setsingle, setdouble, parsingle, pardouble, stempUp, stempDown, sADC, dtempUp, dtempDown, dADC, timer, sburner, dburner};
Numb Path;

const int addr_tempUs = 0;
const int addr_tempDs = 5;
const int addr_tempUd = 10;
const int addr_tempDd = 15;
const int addr_adcs   = 20;
const int addr_adcd   = 25;
const int addr_timer  = 30;

bool state = true, in = true;
char setPoint[4], setPointtime[3], setting, readKey;
int st = 0, cs = 6;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPin[ROWS] = {35,37,39,41};
byte colPin[COLS] = {43,45,47,49};

Keypad keypad = Keypad(makeKeymap(keys), rowPin, colPin, ROWS, COLS);

//========================Function================================

void SingleBurner(){
  switch(StepSingle){                             
    case start      : readKey = keypad.getKey();
                      if(readKey == 'A' || state == false){
                        state = true;
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Blow:ON");
                        Start.lastTime = millis();
                        digitalWrite(Blow, LOW);
                        StepSingle = process;
                      }
                      break;
                   
    case process    : Start.currentTime = millis();
                      if((Start.currentTime - Start.lastTime) > 4000 && (Start.currentTime - Start.lastTime) <= 5000){
                        lcd.setCursor(8,0);
                        lcd.print("|| Ig:ON");
                        digitalWrite(Ig, LOW);
                      }
                      if((Start.currentTime - Start.lastTime) > 9000 && (Start.currentTime - Start.lastTime) <= 14000){
                        Flame1.value = analogRead(sens1);
                        Flame2.value = analogRead(sens2);
                        lcd.setCursor(5,1);
                        lcd.print("Sel:ON");
                        digitalWrite(Sel1, LOW);
                        
                        if(Flame1.value <= Single.adc && Flame2.value <= Single.adc){
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("Suhu:");
                          lcd.setCursor(6,0);
                          lcd.print("U");
                          lcd.setCursor(7,0);
                          lcd.print(Single.tempU);
                          lcd.setCursor(11,0);
                          lcd.print("D");
                          lcd.setCursor(12,0);
                          lcd.print(Single.tempD);
                          digitalWrite(Ig, HIGH);
                          in = true;
                          StepSingle = temperatur;
                        }
                      }
                      
                      if((Start.currentTime - Start.lastTime) > 14000){
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Ig:OFF||Sel:OFF");
                        Blower.lastTime = millis();
                        StepSingle = buzzer;
                      }
                      break;

    case temperatur : lcd.setCursor(5,1);
                      lcd.print(thermocp.readCelsius());
     
                      if(thermocp.readCelsius() > Single.tempU){
                        digitalWrite(Sel1, HIGH);
                        in = false;
                      }
                      
                      if(thermocp.readCelsius() <= Single.tempD && in == false){
                        state = false;
                        StepSingle = start;
                      }

                      if(thermocp.readCelsius() <= Single.tempD && in == true){
                        Flame1.value = analogRead(sens1);
                        Flame2.value = analogRead(sens2);
                        if(Flame1.value >= Single.adc || Flame2.value >= Single.adc){
                          lcd.clear();
                          lcd.setCursor(4,0);
                          lcd.print("Sel:OFF");
                          Blower.lastTime = millis();
                          StepSingle = buzzer;
                        }
                      }                 
                      delay(500);
                      break;

   case buzzer      : Blower.currentTime = millis();
                      if((Blower.currentTime - Blower.lastTime) > 5000){
                        lcd.setCursor(4,1);
                        lcd.print("Blow:OFF");
                        digitalWrite(Blow, HIGH);
                        digitalWrite(buzz, HIGH);
                        delay(200);
                        digitalWrite(buzz, LOW);
                        delay(200);
                        if(keypad.getKey() == 'A'){
                          state = false;
                          StepSingle = start;
                        }
                      }
                      
                      else{
                        digitalWrite(Sel1, HIGH);
                        digitalWrite(Ig, HIGH); 
                      }
                      break;
  }
}

void DoubleBurner(){
  switch(StepDouble){
    case start      : readKey = keypad.getKey();
                      if(readKey == 'A' || state == false){
                        state = true;
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Blow:ON");
                        Start.lastTime = millis();
                        digitalWrite(Blow, LOW);
                        StepDouble = process;
                      }
                      break;
                   
    case process    : Start.currentTime = millis();
                      if((Start.currentTime - Start.lastTime) > 4000 && (Start.currentTime - Start.lastTime) <= 5000){
                        lcd.setCursor(7,0);
                        lcd.print("||Ig:ON");
                        digitalWrite(Ig, LOW);
                      }
                      
                      if((Start.currentTime - Start.lastTime) > 9000 && (Start.currentTime - Start.lastTime) <= 14000){
                        Flame1.value = analogRead(sens1);
                        Flame2.value = analogRead(sens2);
                        lcd.setCursor(0,1);
                        lcd.print("Sel1:ON");
                        digitalWrite(Sel1, LOW);
                        
                        if(Flame1.value <= Double.adc && Flame2.value <= Double.adc){
                          digitalWrite(Ig, HIGH);
                          in = true;
                          Burner.lastTime = millis();
                          StepDouble = burner;
                        }
                      }
                   
                      if((Start.currentTime - Start.lastTime) > 14000){
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Ig:OFF||Sel1:OFF");
                        Blower.lastTime = millis();
                        StepDouble = buzzer;
                      }
                      break;

    case temperatur : lcd.setCursor(5,1);
                      lcd.print(thermocp.readCelsius());
     
                      if(thermocp.readCelsius() > Double.tempU){
                        digitalWrite(Sel2, HIGH);
                        in = false;
                      }
                      
                      if(thermocp.readCelsius() <= Double.tempD && in == false){
                        digitalWrite(Sel2, LOW);
                      }

                      Flame1.value = analogRead(sens1);
                      Flame2.value = analogRead(sens2);
                      if(Flame1.value >= Double.adc || Flame2.value >= Double.adc){
                        lcd.clear();
                        lcd.setCursor(1,0);
                        lcd.print("Sel1&Sel2:OFF");
                        digitalWrite(Sel2, HIGH);
                        Blower.lastTime = millis();
                        StepDouble = buzzer;
                      }
                      delay(500);
                      break;

    case buzzer     : Blower.currentTime = millis();
                      if((Blower.currentTime - Blower.lastTime) > 5000){
                        lcd.setCursor(4,1);
                        lcd.print("Blow:OFF");
                        digitalWrite(Blow, HIGH);
                        digitalWrite(buzz, HIGH);
                        delay(200);
                        digitalWrite(buzz, LOW);
                        delay(200);
                        if(keypad.getKey() == 'A'){
                          state = false;
                          StepDouble = start;
                        }
                      }
                      
                      else{
                        digitalWrite(Sel1, HIGH);
                        digitalWrite(Ig, HIGH); 
                      }
                      break;

    case burner     : Burner.currentTime = millis();
                      if((Burner.currentTime - Burner.lastTime) > Double.timer){
                        lcd.setCursor(7,1);
                        lcd.print("||Sel2:ON");
                        digitalWrite(Sel2, LOW);
                        delay(1000);
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Suhu:");
                        lcd.setCursor(6,0);
                        lcd.print("U");
                        lcd.setCursor(7,0);
                        lcd.print(Double.tempU);
                        lcd.setCursor(11,0);
                        lcd.print("D");
                        lcd.setCursor(12,0);
                        lcd.print(Double.tempD);
                        StepDouble = temperatur;
                      }

                      Flame1.value = analogRead(sens1);
                      Flame2.value = analogRead(sens2);
                      if(Flame1.value >= Double.adc || Flame2.value >= Double.adc){
                        lcd.clear();
                        lcd.setCursor(4,0);
                        lcd.print("Sel1:OFF");
                        Blower.lastTime = millis();
                        StepDouble = buzzer;
                      }
                      break;
  }
}

void MainProgram(){
  switch(Path){
    case mode     : lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Single Burner(1)");
                    lcd.setCursor(0,1);
                    lcd.print("Double Burner(2)");
                    Path = cmode;
                    break;
                  
    case cmode    : readKey = keypad.getKey();
                    if(readKey == '1'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("SetB(*)|SetL(#)");
                      lcd.setCursor(5,1);
                      lcd.print("Def(0)");
                      delay(50);
                      Path = setsingle;
                    }
                  
                    if(readKey == '2'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("SetB(*)|SetL(#)");
                      lcd.setCursor(5,1);
                      lcd.print("Def(0)");
                      delay(50);
                      Path = setdouble;
                    }
                    break;
                   
    case setsingle: readKey = keypad.getKey();
                    if(readKey == '*'){
                      lcd.clear();
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      delay(50);
                      Path = parsingle; 
                    }

                    if(readKey == '0'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Single.tempU = 100;
                      Single.tempD = 95;
                      Single.adc   = 950;
                      EEPROM.put(addr_tempUs, Single.tempU);
                      EEPROM.put(addr_tempDs, Single.tempD);
                      EEPROM.put(addr_adcs, Single.adc);
                      delay(50);
                      Path = sburner; 
                    }

                    if(readKey == '#'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Single.tempU = Single.rtempU;
                      Single.tempD = Single.rtempD;
                      Single.adc   = Single.radc;
                      delay(50);
                      Path = sburner;
                    }
  
                    if(readKey == 'B'){
                      lcd.clear();
                      delay(50);
                      Path = mode;
                    }
                    break;

    case setdouble: readKey = keypad.getKey();
                    if(readKey == '*'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      delay(50);
                      Path = pardouble; 
                    }

                    if(readKey == '0'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Double.tempU  = 100;
                      Double.tempD  = 95;
                      Double.adc    = 950;
                      Double.timer  = 4000;
                      Double.rtimer = 4000/1000; 
                      EEPROM.put(addr_tempUd, Double.tempU);
                      EEPROM.put(addr_tempDd, Double.tempD);
                      EEPROM.put(addr_adcd, Double.adc);
                      EEPROM.put(addr_timer, Double.rtimer);
                      delay(50);
                      Path = dburner; 
                    }

                    if(readKey == '#'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Double.tempU = Double.rtempU;
                      Double.tempD = Double.rtempD;
                      Double.adc   = Double.radc;
                      Double.timer = (unsigned long)Double.rtimer * 1000;
                      delay(50);
                      Path = dburner;
                    }
   
                    if(readKey == 'B'){
                      lcd.clear();
                      delay(50);
                      Path = mode;
                    }
                    break;

    case parsingle: readKey = keypad.getKey();
                    if(readKey == '1'){
                      lcd.clear();
                      delay(50);
                      Path = stempUp;
                    }

                    if(readKey == '2'){
                      lcd.clear();
                      delay(50);
                      Path = stempDown;
                    }

                    if(readKey == '3'){
                      lcd.clear();
                      delay(50);
                      Path = sADC;
                    }

                    if(readKey == '#'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Single.tempU = Single.rtempU;
                      Single.tempD = Single.rtempD;
                      Single.adc   = Single.radc;
                      delay(50);
                      Path = sburner;
                    }
                
                    if(readKey == 'B'){
                      lcd.clear();
                      delay(50);
                      Path = mode;
                    }
                    break; 

    case pardouble: readKey = keypad.getKey();
                    if(readKey == '1'){
                      lcd.clear();
                      delay(50);
                      Path = dtempUp;
                    }

                    if(readKey == '2'){
                      lcd.clear();
                      delay(50);
                      Path = dtempDown;
                    }

                    if(readKey == '3'){
                      lcd.clear();
                      delay(50);
                      Path = dADC;
                    }

                    if(readKey == '4'){
                      lcd.clear();
                      delay(50);
                      Path = timer;
                    }

                    if(readKey == '#'){
                      lcd.clear();
                      lcd.setCursor(3,0);
                      lcd.print("--Ready--");
                      lcd.setCursor(0,1);
                      lcd.print("Start Burner(A)");
                      Double.tempU = Double.rtempU;
                      Double.tempD = Double.rtempD;
                      Double.adc   = Double.radc;
                      Double.timer = (unsigned long)Double.rtimer * 1000;
                      delay(50);
                      Path = dburner;
                    }
                    
                    if(readKey == 'B'){
                      lcd.clear();
                      delay(50);
                      Path = mode;
                    }
                    break;

    case stempUp  : lcd.setCursor(1,0);
                    lcd.print("Suhu Atas(C):");
                    lcd.setCursor(0,1);
                    lcd.print(Single.rtempU);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Single.rtempU  = atoi(setPoint);
                      Single.rtempU  = constrain(Single.rtempU, 30, 1000);
                      EEPROM.put(addr_tempUs, Single.rtempU); 
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case stempDown: lcd.setCursor(1,0);
                    lcd.print("Suhu Bawah(C):");
                    lcd.setCursor(0,1);
                    lcd.print(Single.rtempD);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Single.rtempD  = atoi(setPoint);
                      Single.rtempD  = constrain(Single.rtempD, 28, 1000);
                      EEPROM.put(addr_tempDs, Single.rtempD); 
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case sADC     : lcd.setCursor(0,0);
                    lcd.print("Set ADC (Calib):");
                    lcd.setCursor(0,1);
                    lcd.print(Single.radc);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Single.radc  = atoi(setPoint);
                      Single.radc  = constrain(Single.radc, 0, 1024);
                      EEPROM.put(addr_adcs, Single.radc); 
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(1,0);
                      lcd.print("Up(1)|Down(2)");
                      lcd.setCursor(5,1);
                      lcd.print("ADC(3)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = parsingle;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case dtempUp  : lcd.setCursor(1,0);
                    lcd.print("Suhu Atas(C):");
                    lcd.setCursor(0,1);
                    lcd.print(Double.rtempU);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Double.rtempU  = atoi(setPoint);
                      Double.rtempU  = constrain(Double.rtempU, 30, 1000);
                      EEPROM.put(addr_tempUd, Double.rtempU); 
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case dtempDown: lcd.setCursor(1,0);
                    lcd.print("Suhu Bawah(C):");
                    lcd.setCursor(0,1);
                    lcd.print(Double.rtempD);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Double.rtempD  = atoi(setPoint);
                      Double.rtempD  = constrain(Double.rtempD, 28, 1000);
                      EEPROM.put(addr_tempDd, Double.rtempD); 
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case dADC     : lcd.setCursor(0,0);
                    lcd.print("Set ADC (Calib):");
                    lcd.setCursor(0,1);
                    lcd.print(Double.radc);
                    lcd.setCursor(4,1);
                    lcd.print("|");
                    setPoint[st] = keypad.getKey();
                    if(setPoint[st] == '*' || setPoint[st] == 'A' || setPoint[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPoint[st] == 'D'){
                      lcd.setCursor(5,1);
                      lcd.print("           ");
                      st = 0;
                      cs = 6;
                    }

                    else if(setPoint[st] == '#'){
                      lcd.clear();
                      Double.radc  = atoi(setPoint);
                      Double.radc  = constrain(Double.radc, 0, 1024);
                      EEPROM.put(addr_adcd, Double.radc); 
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }

                    else if(setPoint[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (3)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }
                      
                    else if(setPoint[st] == '0' || setPoint[st] == '1' || setPoint[st] == '2' || setPoint[st] == '3' || setPoint[st] == '4' || setPoint[st] == '5' || setPoint[st] == '6' || setPoint[st] == '7' || setPoint[st] == '8' || setPoint[st] == '9'){
                      if(setPoint[0] == '0'){
                        lcd.setCursor(5,1);
                        lcd.print("           ");
                      }
                        
                      else{
                        if(cs > 9){
                          lcd.setCursor(10,1);
                          lcd.print(" ");
                          st = 4;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPoint[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case timer    : lcd.setCursor(0,0);
                    lcd.print("Set Timer S1-S2:");
                    lcd.setCursor(0,1);
                    lcd.print(Double.rtimer);
                    lcd.setCursor(3,1);
                    lcd.print("|");
                    setPointtime[st] = keypad.getKey();
                    if(setPointtime[st] == '*' || setPointtime[st] == 'A' || setPointtime[st] == 'C'){
                      lcd.setCursor(cs,1);
                      lcd.print(" ");
                    }
                
                    else if(setPointtime[st] == 'D'){
                      lcd.setCursor(4,1);
                      lcd.print("            ");
                      st = 0;
                      cs = 6;
                    }
                    
                    else if(setPointtime[st] == '#'){
                      lcd.clear();
                      Double.rtimer = atoi(setPointtime);
                      Double.rtimer = constrain(Double.rtimer, 0, 120);
                      EEPROM.put(addr_timer, Double.rtimer); 
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }

                    else if(setPointtime[st] == 'B'){
                      lcd.clear();
                      lcd.setCursor(0,0);
                      lcd.print("Up (1)|Down (2)");
                      lcd.setCursor(0,1);
                      lcd.print("ADC(3)|Timer(4)");
                      st = 0;
                      cs = 6;
                      delay(50);
                      Path = pardouble;
                    }
                      
                    else if(setPointtime[st] == '0' || setPointtime[st] == '1' || setPointtime[st] == '2' || setPointtime[st] == '3' || setPointtime[st] == '4' || setPointtime[st] == '5' || setPointtime[st] == '6' || setPointtime[st] == '7' || setPointtime[st] == '8' || setPointtime[st] == '9'){
                      if(setPointtime[0] == '0'){
                        lcd.setCursor(4,1);
                        lcd.print("            ");
                      }
                        
                      else{
                        if(cs > 8){
                          lcd.setCursor(9,1);
                          lcd.print(" ");
                          st = 2;
                        }
                      
                        else{
                          lcd.setCursor(cs,1);
                          lcd.print(setPointtime[st]); 
                        }
                      st++;
                      cs++;
                      }
                    }
                    break;

    case sburner  : SingleBurner();
                    break;

    case dburner  : DoubleBurner();
                    break;
  }
}

void getEEPROM(){
  EEPROM.get(addr_tempUs, Single.rtempU);
  EEPROM.get(addr_tempDs, Single.rtempD);
  EEPROM.get(addr_adcs, Single.radc); 
  
  EEPROM.get(addr_tempUd, Double.rtempU);
  EEPROM.get(addr_tempDd, Double.rtempD);
  EEPROM.get(addr_adcd, Double.radc); 
  EEPROM.get(addr_timer, Double.rtimer);
}

void setup() {
  lcd.begin();
  pinMode(Sel1, OUTPUT);
  pinMode(Sel2, OUTPUT);
  pinMode(Blow, OUTPUT);
  pinMode(Ig, OUTPUT);
  digitalWrite(Sel1, HIGH);
  digitalWrite(Sel2, HIGH);
  digitalWrite(Blow, HIGH);
  digitalWrite(Ig, HIGH);
  pinMode(buzz, OUTPUT);
  pinMode(sens1, INPUT);
  pinMode(sens2, INPUT);
  getEEPROM();
  delay(50);
  lcd.setCursor(1,0);
  lcd.print("Burner Control");
  lcd.setCursor(5,1);
  lcd.print("System");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("--Choose Mode--");
  delay(1000);
}

void loop() {
  MainProgram();
}
