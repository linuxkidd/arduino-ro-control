#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display1(OLED_RESET);
Adafruit_SSD1306 display2(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const char disp_lines[][11]= { "", "      Pump", "     Fresh", "      Grey" };
const char state_opts[][4][12] = { 
  { "Enable RO ", " Force RO ", "    OFF   " },
  { " Idle   ", " On     ", " Force  " },
  { " Low    ", " High   " } };

unsigned int cur_state[4] = { 2, 0, 0, 0 };

#define FRESH_PIN 2
#define  GREY_PIN 3
#define    SW_PIN 4
#define  PUMP_PIN 5

bool disp_update=1;

int i=0;
int so=0;
long lastDebounceTime=0;
long debounceDelay=50;
long enactSelectionDelay=3000;
long lastSwitchTime=0;

bool last_sw_state=1;
bool sw_need_update=1;
bool force_pump=1;

void setup()   {                
  Serial.begin(9600);

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(FRESH_PIN, INPUT_PULLUP);
  pinMode(GREY_PIN, INPUT_PULLUP);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display2.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Clear the buffer.
  display1.clearDisplay();
  display2.clearDisplay();
  display1.display();
  display2.display();
}

void loop() {
  checkSW();
  checkFloat(FRESH_PIN);
  checkFloat(GREY_PIN);

  if(lastSwitchTime!=0 && millis()-lastSwitchTime>enactSelectionDelay || force_pump) {
    setPump();
    lastSwitchTime=0;
    force_pump=0;
  }
  if(disp_update==1) {
    //updateSerial();
    updateDisplay();
  }
  disp_update=0;
}

void checkSW() {
  int reading = digitalRead(SW_PIN);
  if (reading != last_sw_state && !sw_need_update) {
    lastDebounceTime = millis();
    sw_need_update=1;
  }

  if((millis() - lastDebounceTime) > debounceDelay && sw_need_update) {
    if(reading==LOW) {
      disp_update=1;
      cur_state[0]++;
      if(cur_state[0]>2)
        cur_state[0]=0;
      lastSwitchTime=millis();
    }
    sw_need_update=0;
  }
  last_sw_state=reading;
}

void checkFloat(int floatID) {
  int lastState=cur_state[floatID];
  cur_state[floatID] = !digitalRead(floatID);
  if(cur_state[floatID]!=lastState) {
    disp_update=1;
    force_pump=1;
  }
}

void setPump() {
  int lastState=cur_state[1];
  cur_state[1]=0;
  if(cur_state[0]<2) {
    if(cur_state[0]==1) {
      cur_state[1]=2;
    } else {
      if(cur_state[2]==cur_state[3] && cur_state[3]==0)
        cur_state[1]=1;
    }
  }
  if(lastState!=cur_state[1]) {
    digitalWrite(PUMP_PIN, cur_state[1]);
    disp_update=1;
  }
}
void updateDisplay() {
  char outputbuffer[14];

  display1.clearDisplay();
  display2.clearDisplay();
  display1.setCursor(0,0);
  display2.setCursor(0,0);

  for(int i=0;i<4;i++) {
    if(i==0) {
      display1.setTextSize(2);
      display2.setTextSize(2);
      sprintf(outputbuffer,"%s",state_opts[i][cur_state[i]]);
    } else {
      display1.setTextSize(1);
      display2.setTextSize(1);
      int so=i;
      if(i==3)
        so=2;
      sprintf(outputbuffer,"%s: %s\n",disp_lines[i],state_opts[so][cur_state[i]]);
    }
    if((cur_state[i]>0 && (i==0 || i==GREY_PIN || (i==FRESH_PIN && cur_state[1]>1))) || (cur_state[i]>1 && i==1)) {
      display1.setTextColor(BLACK,WHITE);
      display2.setTextColor(BLACK,WHITE);
    } else {
      display1.setTextColor(WHITE,BLACK);
      display2.setTextColor(WHITE,BLACK);
    }
    display1.print(outputbuffer);
    display2.print(outputbuffer);
    display1.setTextColor(WHITE,BLACK);
    display2.setTextColor(WHITE,BLACK);
    display1.setTextSize(1);
    display2.setTextSize(1);
    display1.print("                     ");
    display2.print("                     ");

  }
  display1.display();
  display2.display();
}

void updateSerial() {
  char outputbuffer[14];

  for(int i=0;i<4;i++) {
    if(i==0) {
      sprintf(outputbuffer,"2: %d, %s",cur_state[i],state_opts[i][cur_state[i]]);
    } else {
      int so=i;
      if(i==3) {
        so=2;
      }
      sprintf(outputbuffer,"1: %d, %s: %s",cur_state[i],disp_lines[i],state_opts[so][cur_state[i]]);
    }
    Serial.println(outputbuffer);
    Serial.println("                 ");
  }
}


