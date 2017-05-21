int buz = 8;
int led = 13;
int ai0 = A0;    // select the input pin for analog input
int ai1 = A1;    // select the input pin for analog input
int ai2 = A2;    // select the input pin for analog input
double PowerInst = 0;   // instantaneous power consumption [W]

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// D7 - Serial clock out (CLK oder SCLK)
// D6 - Serial data out (DIN)
// D5 - Data/Command select (DC oder D/C)
// D4 - LCD chip select (CE oder CS)
// D3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

void setup() {
  pinMode(led, OUTPUT);     // initialize pin as logical OUTPUT
  Serial.begin(38400);       // open the serial port at 38400 bps
  //analogReadResolution(12); // initialize analog risolution (mod. Due and Zero only)
  display.begin();          // initialize display
  display.setContrast(50);  // set contrast around to adapt the display here for best viewing
  display.display();        // show splashscreen
  delay(2000);              // sleep 2s
  display.clearDisplay();   // clears the screen and buffer

  pinMode(buz, OUTPUT);     // initialize pin as logical OUTPUT
  digitalWrite(buz, HIGH);  // Buzzer ON
  delay(300);
  digitalWrite(buz, LOW); // Buzzer OFF
}

void loop() {
  PowerInst = ReadAnlogInput(ai2);
  
  if (PowerInst > 100) {
    PowerInst *= 13.635;
    Serial.println("Analog Input 2");
    Show(PowerInst);
    return;
  }
  else {
    PowerInst = ReadAnlogInput(ai1);
  }
  
  if (PowerInst > 100) {
    PowerInst *= 7.022;
    Serial.println("Analog Input 1");
    Show(PowerInst);
    return;
  }
  else {
    PowerInst = ReadAnlogInput(ai0);
  }

  PowerInst *= 2.671;
  Serial.println("Analog Input 0");
  Show(PowerInst);
}

void Show(int p) {
  display.clearDisplay();
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Power Usage W\n");
  display.setTextSize(2);
  display.print(" ");
  display.print(p);
  display.setTextSize(1);
  display.print("\n\r\n\r");
  display.setTextSize(2);
  
  if (p > 3300) {
    display.println("ALARM!");
    digitalWrite(buz, HIGH);  // Buzzer ON
  }
  else if (p > 3000) {
    display.println(" WARN!");
    digitalWrite(buz, HIGH);  // Buzzer ON
    delay(3325 - p);
    digitalWrite(buz, LOW); // Buzzer OFF
  }
  else {
    display.println("  OK");
    digitalWrite(buz, LOW); // Buzzer OFF
}
  display.display();
  Serial.println(p);
}

long ReadAnlogInput(int aiPin) {
  int num = 1000;
  long aiVal = 0;
  unsigned long sum = 0;
  for (int i = 0; i < num; i++) {
    aiVal = analogRead(aiPin) - 512;  // read the value from the sensor - 512
    if (aiVal != 0) sum += (aiVal * aiVal);
  }
  return sqrt(sum / num);
}

