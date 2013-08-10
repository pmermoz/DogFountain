

#include <NewPing.h>

const int LED_OUTPUT = 13;
const int PIR_INPUT = 7;

//--- HC-SR04

const int HC_SR04_ECHO_PIN = 5;
const int HC_SR04_TRIGGER_PIN = 6;
const int LED_RED_DISTANCE_PIN = A4;
const int LED_GREEN_DISTANCE_PIN = A3;
const int LED_BLUE_DISTANCE_PIN = A2;

//#define TRIGGER_PIN  7  // Arduino pin tied to trigger pin on the ultrasonic sensor.
//#define ECHO_PIN     8  // Arduino pin tied to echo pin on the ultrasonic sensor.
const int MAX_DISTANCE = 100; // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(HC_SR04_TRIGGER_PIN, HC_SR04_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

unsigned long pirDetectionUntilTimeMs;
unsigned long pingFeedbackUntilTimeMs;
unsigned long nextPingTimeMs;

//--- Valve

const int VALVE_1_ON_PIN = 4;
const int VALVE_1_OFF_PIN = 2;
const int VALVE_1_ENABLE_PIN = 3;

static unsigned long lastPIRDetection = 0;
static long distanceCm = 0;

static unsigned long startWaterTime = 0;
static unsigned long periodicValveOff = 0;

//---

void setup()
{

  pinMode(LED_OUTPUT, OUTPUT);
  pinMode(PIR_INPUT, INPUT);

  digitalWrite(LED_OUTPUT, LOW);

  //--- HC-SR04

  pinMode(HC_SR04_TRIGGER_PIN, OUTPUT);
  pinMode(HC_SR04_ECHO_PIN, INPUT);
  pinMode(LED_RED_DISTANCE_PIN, OUTPUT);
  pinMode(LED_GREEN_DISTANCE_PIN, OUTPUT);
  pinMode(LED_BLUE_DISTANCE_PIN, OUTPUT);

  nextPingTimeMs = 0;

  //--- Valve

  pinMode(VALVE_1_ON_PIN, OUTPUT);
  pinMode(VALVE_1_OFF_PIN, OUTPUT);
  pinMode(VALVE_1_ENABLE_PIN, OUTPUT);
  //digitalWrite(VALVE_1_ENABLE_PIN, LOW);

  //valveOff();
  //valveOn();

  //---

  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.

  // RED - > GREEN
  // GREEN -> RED
  // 
  //digitalWrite(LED_RED_DISTANCE_PIN, HIGH);
  //digitalWrite(LED_GREEN_DISTANCE_PIN, HIGH);
  //digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
  //digitalWrite(LED_OUTPUT, HIGH);

  // CH B
  //digitalWrite(VALVE_1_ON_PIN, LOW);
  //digitalWrite(VALVE_1_ON_PIN, HIGH);
  // CH C
  //digitalWrite(VALVE_1_ENABLE_PIN, LOW);
  //digitalWrite(VALVE_1_ENABLE_PIN, HIGH);
  // CH D
  //digitalWrite(VALVE_1_OFF_PIN, LOW);
  //digitalWrite(VALVE_1_OFF_PIN, HIGH);

  //digitalWrite(LED_OUTPUT, HIGH);
}

void loop()
{ 
  //delay(1000);
  //return;

  unsigned long nowTimeMs = millis();

  boolean didPIRDetect = digitalRead(PIR_INPUT);
  if (didPIRDetect) {
    lastPIRDetection = nowTimeMs;
    pirDetectionUntilTimeMs = nowTimeMs + 5 * 1000;
  }

  //--- HC-SR04
  boolean didPing = false;
  boolean didDetectPresence = false;

  if (pirDetectionUntilTimeMs >  nowTimeMs && nowTimeMs > nextPingTimeMs) {
    nextPingTimeMs = nowTimeMs + 1000;

    // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.

    //    long echoTime = sonar.ping_median(3); // Send ping, get ping time in microseconds (uS).
    long echoTime = sonar.ping(); // Send ping, get ping time in microseconds (uS).
    distanceCm = sonar.convert_cm(echoTime);
    didPing = true;
    pingFeedbackUntilTimeMs = nowTimeMs + 200;
  }

  if (pirDetectionUntilTimeMs >  nowTimeMs && distanceCm > 0 && distanceCm < 50) {
    didDetectPresence = true;

    if (startWaterTime == 0) {
      valveOn();
    }
    startWaterTime = nowTimeMs;
  }
  else {
    didDetectPresence = false;
    if (startWaterTime > 0) {
      unsigned long durationTime;
      if (nowTimeMs >= startWaterTime) {
        durationTime = nowTimeMs - startWaterTime;
      }
      else {
        durationTime = nowTimeMs;
      }

      if (durationTime > 2 * 1000) {
        valveOff();
        startWaterTime = 0;
      }
    }
    else if (nowTimeMs - periodicValveOff > 60 * 1000) {
      valveOff();
      periodicValveOff = nowTimeMs;
    }
  }


  //--- Feedback

  if (didDetectPresence) {
    digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
    digitalWrite(LED_RED_DISTANCE_PIN, LOW);
    digitalWrite(LED_GREEN_DISTANCE_PIN, HIGH);
  }
  else {
    if (pirDetectionUntilTimeMs >  nowTimeMs) {
      if (pingFeedbackUntilTimeMs > nowTimeMs) {
        digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
        digitalWrite(LED_RED_DISTANCE_PIN, HIGH);
        digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
      }
      else {
        digitalWrite(LED_RED_DISTANCE_PIN, LOW);
        
        if (startWaterTime > 0) {
          digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
          digitalWrite(LED_GREEN_DISTANCE_PIN, HIGH);
        }
        else {
          digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
          digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
        }
        
        
//        if (didPIRDetect) {
//          digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
//        }
//        else {
//          //analogWrite(LED_BLUE_DISTANCE_PIN, 1);
//          //digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
//
//          digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
//          digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
//          digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
//        }
      }
    }
    else {
      digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
      digitalWrite(LED_RED_DISTANCE_PIN, LOW);
      digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
    }
  }

  //      if (distanceCm == 0) {
  //      analogWrite(LED_RED_DISTANCE_PIN, 0);
  //      analogWrite(LED_GREEN_DISTANCE_PIN, 0);
  //      analogWrite(LED_BLUE_DISTANCE_PIN, 0);
  //    }
  //    else {
  //      Serial.print("Ping: ");
  //      Serial.print(distanceCm); // Convert ping time to distance and print result (0 = outside set distance range, no ping echo)
  //      Serial.println("cm");
  //
  //      int redLightLevel = 0;
  //      int greenLightLevel = 0;
  //      int blueLightLevel = 0;
  //      if (distanceCm < 10) {
  //        //redLightLevel = map(distance, 0, 100, 255, 0);
  //        redLightLevel = map(distanceCm, 0, 10, 255, 50);
  //      }
  //      else if (distanceCm < 30) {
  //        //redLightLevel = map(distance, 200, 300, 255, 0);
  //        //blueLightLevel = map(distance, 200, 300, 0, 255);
  //        redLightLevel = map(distanceCm, 10, 30, 150, 50); //150;
  //        greenLightLevel = map(distanceCm, 10, 30, 255, 155); //200;
  //      }
  //      else if (distanceCm < 50) {
  //        //redLightLevel = map(distance, 200, 300, 255, 0);
  //        //blueLightLevel = map(distance, 200, 300, 0, 255);
  //        greenLightLevel = map(distanceCm, 30, 50, 150, 255);
  //      }
  //      else  {
  //        //blueLightLevel = map(distance, 300, MAX_DISTANCE*10, 255, 0);
  //        //greenLightLevel = map(distance, 300, MAX_DISTANCE*10, 0, 255);
  //        blueLightLevel = map(distanceCm, 50, MAX_DISTANCE, 255, 10);
  //      }
  //
  //      //      Serial.print("LED: R(");
  //      //      Serial.print(redLightLevel);
  //      //      Serial.print(") G(");
  //      //      Serial.print(greenLightLevel);
  //      //      Serial.print(") B(");
  //      //      Serial.print(blueLightLevel);
  //      //      Serial.println(")");
  //
  //      analogWrite(LED_RED_DISTANCE_PIN, redLightLevel);
  //      analogWrite(LED_GREEN_DISTANCE_PIN, greenLightLevel);
  //      analogWrite(LED_BLUE_DISTANCE_PIN, blueLightLevel);
  //    }

}
//   80ms ->  800mA
//  200ms  1017mA
//  300ms  1018mA
void valveOn() {
  digitalWrite(VALVE_1_ON_PIN, HIGH);
  digitalWrite(VALVE_1_OFF_PIN, LOW);
  digitalWrite(VALVE_1_ENABLE_PIN, HIGH);
  delay(20);
  digitalWrite(VALVE_1_ON_PIN, LOW);
  digitalWrite(VALVE_1_ENABLE_PIN, LOW);

  digitalWrite(LED_OUTPUT, HIGH);
}

void valveOff() {
  digitalWrite(VALVE_1_ON_PIN, LOW);
  digitalWrite(VALVE_1_OFF_PIN, HIGH);
  digitalWrite(VALVE_1_ENABLE_PIN, HIGH);
  delay(50);
  digitalWrite(VALVE_1_OFF_PIN, LOW);
  digitalWrite(VALVE_1_ENABLE_PIN, LOW);

  digitalWrite(LED_OUTPUT, LOW);
}


