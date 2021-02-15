#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define DHT1 5
#define DHT2 6
#define CLK 2
#define DT 3
#define SW 4
#define FAN 7

DHT dht1(DHT1, DHT11);
DHT dht2(DHT2, DHT11);
LiquidCrystal_I2C lcd(0x3F,16,2);

unsigned long last_DHT_update = 0, last_EEPROM_update = 0;
char lcdChar[16];
int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;
bool isTempSelected = 1;
byte t_target = EEPROM.read(0), h_target = EEPROM.read(1), t_offset = 1, h_offset = 10;

typedef struct{
  float tmp1;
  float hum1;
  float tmp2;
  float hum2;
  float temp;
  float hum;
}DHTs;

DHTs dht;


void setup() {
  Serial.begin(9600);
  dht1.begin();
  dht2.begin();
  lcd.begin();
  lcd.clear();         
  lcd.backlight();

  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(FAN, OUTPUT);

  lastStateCLK = digitalRead(CLK);
}

void loop() {
  sprintf(lcdChar, "T %d H %d ", t_target, h_target);
  lcd.setCursor(0,0);
  lcd.print(lcdChar);
  
  get_DHT_val();
  lcdPrint(1, dht.temp, dht.hum); //display the actual value at line 2 of lcd.

  if(dht.temp > t_target+t_offset || dht.hum > h_target+h_offset){
    digitalWrite(FAN, HIGH);
  } else if((int)dht.temp == t_target-t_offset || (int)dht.hum == h_target-h_offset){
    digitalWrite(FAN, LOW);
  }
  
  save_settings();
}

void get_DHT_val(){
  if(millis()-last_DHT_update > 2000){// get DHT value every 2 seconds interval
    float h1 = dht1.readHumidity();
    float t1 = dht1.readTemperature();
    float h2 = dht2.readHumidity();
    float t2 = dht2.readTemperature();
  
    if (isnan(h1) || isnan(t1) || isnan(h2) || isnan(t2)) { //don't accept null values
      Serial.println("Failed to read from DHT sensor!");
      return;
    } else {
      dht.tmp1 = t1;
      dht.hum1 = h1;
      dht.tmp2 = t2;
      dht.hum2 = h2;
      dht.temp = (t1+t2)/2;
      dht.hum = (h1+h2)/2;
    }
    last_DHT_update = millis();
  }
}

void lcdPrint(int y, float t, float h){
  char str_tmp[5], str_hum[5];
  dtostrf(t, 4, 2, str_tmp);
  dtostrf(h, 4, 2, str_hum);
  sprintf(lcdChar, "T %s H %s ", str_tmp, str_hum);
  lcd.setCursor(0,y);
  lcd.print(lcdChar);
}

void save_settings(){
  if(millis()-last_EEPROM_update > 30000){
    EEPROM.update(0, t_target);
    EEPROM.update(1, h_target);
    last_EEPROM_update = millis();
  }
}

void read_rotary(){
  currentStateCLK = digitalRead(CLK);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK) {
      if(isTempSelected){
        t_target--;
      }else{
        h_target--;
      }
      currentDir ="CCW";
    } else {
      // Encoder is rotating CW so increment
      if(isTempSelected){
        t_target++;
      }else{
        h_target++;
      }
      currentDir ="CW";
    }
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  // Read the button state
  int btnState = digitalRead(SW);

  //If we detect LOW signal, button is pressed
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {
      isTempSelected = !isTempSelected;
    }

    // Remember last button press event
    lastButtonPress = millis();
  }

  // Put in a slight delay to help debounce the reading
  delay(1);
}
