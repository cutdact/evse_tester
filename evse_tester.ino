/*
 * 
 * Simple EVSE tester by Martin Jansson
 * 
 */
/*---------------------------------------------------------------------------*/
#include <ArduinoJson.h>
 /*---------------------------------------------------------------------------*/
#define SAMPLE_PERIOD 500
#define DIGITAL_COMM_REQD 999999
/*---------------------------------------------------------------------------*/
#define RESISTOR_2K7_PIN  9
#define RESISTOR_1K3_PIN  7
#define RESISTOR_0K3_PIN  4
#define CONTROL_PILOT_IN  A0
/*---------------------------------------------------------------------------*/
char state[] = {'N'};
/*---------------------------------------------------------------------------*/
int interval=1000;
unsigned long previousMillis=0;
/*---------------------------------------------------------------------------*/

// duty is tenths-of-a-percent (that is, fraction out of 1000).
inline unsigned long dutyToMA(unsigned long duty) {
  // Cribbed from the spec - grant a +/-2% slop
  if (duty < 30) {
    // < 3% is an error
    return 0;
  }
  else if (duty <= 70) {
    // 3-7% - digital
    return DIGITAL_COMM_REQD;
  }
  else if (duty < 80) {
    // 7-8% is an error
    return 0;
  }
  else if (duty <= 100) {
    // 8-10% is 6A
    return 6000;
  }
  else if (duty <= 850) { // 10-85% uses the "low" function
    return duty * 60;
  } 
  else if (duty <= 960) { // 85-96% uses the "high" function
    return (duty - 640) * 250;
  }
  else if (duty <= 970) {
    // 96-97% is 80A
    return 80000;
  }
  else { // > 97% is an error
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
void setup() {
  Serial.begin(9600);
  pinMode(RESISTOR_2K7_PIN, OUTPUT);
  pinMode(RESISTOR_1K3_PIN, OUTPUT);
  pinMode(RESISTOR_0K3_PIN, OUTPUT);

  pinMode(CONTROL_PILOT_IN, INPUT);
  Serial.println("Simple EVSE Tester");
}
/*---------------------------------------------------------------------------*/
void loop() {

  StaticJsonDocument<200> doc;
  unsigned long currentMillis = millis();

  unsigned int last_state = 99; // neither HIGH nor LOW
  unsigned long high_count = 0, low_count = 0, state_changes = -1; // ignore the first change from "invalid"
  
  for(unsigned long start_poll = millis(); millis() - start_poll < SAMPLE_PERIOD; ) 
  {
    unsigned int state = digitalRead(CONTROL_PILOT_IN);
    
    if (state == LOW)
    {
      low_count++;
    }
    else
    {
      high_count++;
    } 
    if (state != last_state) 
    {
      state_changes++;
      last_state = state;
    }
  }
  unsigned long amps = 0;

  unsigned int duty = (high_count * 1000) / (high_count + low_count);
  duty %= 1000; // turn 100% into 0% just for display purposes. A 100% duty cycle doesn't really make sense.

  unsigned long frequency = (state_changes / 2) * (1000 / SAMPLE_PERIOD);

  amps = dutyToMA(duty);
  
  if (Serial.available() > 0) 
  {
    char command = Serial.read();

    switch (command)
    {
      case 'a':
      case 'A':
        digitalWrite(RESISTOR_2K7_PIN, LOW);
        digitalWrite(RESISTOR_1K3_PIN, LOW);
        digitalWrite(RESISTOR_0K3_PIN, LOW);
        state[0] = 'A';
        break;
      case 'b':
      case 'B':
        digitalWrite(RESISTOR_2K7_PIN, HIGH);
        digitalWrite(RESISTOR_1K3_PIN, LOW);
        digitalWrite(RESISTOR_0K3_PIN, LOW);
        state[0] = 'B';
        break;
      case 'c':
      case 'C':
        digitalWrite(RESISTOR_2K7_PIN, HIGH);
        digitalWrite(RESISTOR_1K3_PIN, HIGH);
        digitalWrite(RESISTOR_0K3_PIN, LOW);
        state[0] = 'C';
        break;
      case 'd':
      case 'D':
        digitalWrite(RESISTOR_2K7_PIN, HIGH);
        digitalWrite(RESISTOR_1K3_PIN, HIGH);
        digitalWrite(RESISTOR_0K3_PIN, HIGH);
        state[0] = 'D';
        break;
      default:
      //invalid command
        break;
    }
  }

  if ((unsigned long)(currentMillis - previousMillis) >= interval) 
  {
    doc["state"] = state;
    doc["charge current mA"] = (amps/1000);
    doc["duty cycle percent"] = (duty/10);
    serializeJson(doc, Serial);

    Serial.println();
  
  previousMillis = currentMillis;
  }
}
