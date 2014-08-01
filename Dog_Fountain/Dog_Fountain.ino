

#include <NewPing.h>

//--- PIR
const int PIR_INPUT = 7;
const long PIR_DETECTION_DURATION = 5 * 1000;

//--- Sonar (HC-SR04)
const int HC_SR04_ECHO_PIN = 5;
const int HC_SR04_TRIGGER_PIN = 6;

const int LED_RED_DISTANCE_PIN = 9;
const int LED_GREEN_DISTANCE_PIN = 10;
const int LED_BLUE_DISTANCE_PIN = 11;

const long PING_EVERY = 500;
const long PING_FEEDBACK_DURATION = 200;
const long SONAR_DETECTION_DURATION = 5000;
const int SONAR_DETECTION_DISTANCE = 60;
const int SONAR_MAX_DISTANCE = SONAR_DETECTION_DISTANCE + (SONAR_DETECTION_DISTANCE / 5) ; // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

//--- Valve
const int VALVE_1_ON_PIN = 4;
const int VALVE_1_OFF_PIN = 2;
const int SWITCH_DELAY_MS = 50;

const long MIN_WATERING_DURATION = 5 * 1000;


NewPing sonar(HC_SR04_TRIGGER_PIN, HC_SR04_ECHO_PIN, SONAR_MAX_DISTANCE); // NewPing setup of pins and maximum distance.

unsigned long pirDetectionUntilTimeMs;
unsigned long sonarDetectionUntilTimeMs;
unsigned long wateringUntilTimeMs;

unsigned long nextPingTimeMs;

boolean didDetectPresence = false;
boolean didDetectFarePresence = false;

long distanceCm = 0;

boolean isWatering = false;

//---

void setup()
{
  //--- PIR

  pinMode(PIR_INPUT, INPUT);

  //--- Sonar

  pinMode(HC_SR04_TRIGGER_PIN, OUTPUT);
  pinMode(HC_SR04_ECHO_PIN, INPUT);
  pinMode(LED_RED_DISTANCE_PIN, OUTPUT);
  pinMode(LED_GREEN_DISTANCE_PIN, OUTPUT);
  pinMode(LED_BLUE_DISTANCE_PIN, OUTPUT);

  nextPingTimeMs = 0;

  //--- Valve

  pinMode(VALVE_1_ON_PIN, OUTPUT);
  pinMode(VALVE_1_OFF_PIN, OUTPUT);
  digitalWrite(VALVE_1_ON_PIN, HIGH);
  digitalWrite(VALVE_1_OFF_PIN, HIGH);

  valveOff();
}

void loop()
{ 
  unsigned long nowTimeMs = millis();

  //--- PIR

  boolean didPIRDetect = digitalRead(PIR_INPUT);
  if (didPIRDetect) {
    // PIR detection is valid until pirDetectionUntilTimeMs
    pirDetectionUntilTimeMs = nowTimeMs + PIR_DETECTION_DURATION;
  }

  //--- Sonar

  // To start Sonar ping we need PIR detection (to limit usage of sonar and reduce sonar life)
  // Sonar successfull detection will start watering
  // Keep Sonar while PIR or Sonar recently get detection
  // No Sonar detection for a while will trigger stop watering (after water timeout)
  // Watering have a minimal duration (timeout) to avoid rapid pulse of water (not good for relay... and dog)
  if (pirDetectionUntilTimeMs > nowTimeMs || sonarDetectionUntilTimeMs > nowTimeMs) {
    if (nowTimeMs > nextPingTimeMs) {
      // Wait 500ms between pings (about 2 pings/sec). 29ms should be the shortest delay between pings.
      nextPingTimeMs = nowTimeMs + PING_EVERY;

      didDetectPresence = false;
      didDetectFarePresence = false;

      //long echoTime = sonar.ping_median(3); // Send ping, get ping time in microseconds (uS).
      long echoTime = sonar.ping(); // Send ping, get ping time in microseconds (uS).
      distanceCm = sonar.convert_cm(echoTime);

      if (distanceCm > 0) {
        if (distanceCm < SONAR_DETECTION_DISTANCE) {
          didDetectPresence = true;

          // Sonar detection is valid until sonarDetectionUntilTimeMs
          sonarDetectionUntilTimeMs  = nowTimeMs + SONAR_DETECTION_DURATION;

          if (!isWatering) {
            valveOn();
            wateringUntilTimeMs = nowTimeMs + MIN_WATERING_DURATION;
          }
        }
        else {
          didDetectFarePresence = true;
        }
      }
      if (isWatering && nowTimeMs > sonarDetectionUntilTimeMs && nowTimeMs > wateringUntilTimeMs) {
        valveOff();
      }
    }
    // No else, we do not stop water between ping...
  }
  else {
    didDetectPresence = false;
    didDetectFarePresence = false;
    if (isWatering && nowTimeMs > wateringUntilTimeMs) {
      valveOff();
    }
  }

  //--- Feedback
  
  // Pulsing white    Sleep
  // RED              PIR detection
  // BLUE             PIR was detected
  // Light GREEN      Fare presence detected
  // GREEN            Presence detected during last SONAR_DETECTION_DURATION

  if (pirDetectionUntilTimeMs < nowTimeMs && sonarDetectionUntilTimeMs < nowTimeMs) {
    // Nothing
    int pulsePeriod = 6 * 1000;
    int pulseHalfPeriod = pulsePeriod / 2;
    int feedbackStep = (nowTimeMs % pulsePeriod);
    int lightLevel;
    if (feedbackStep < pulseHalfPeriod) {
      lightLevel = map(feedbackStep, 0, pulseHalfPeriod, 0, 127);
    }
    else {
      lightLevel = map(feedbackStep, pulseHalfPeriod, pulsePeriod, 127, 0);
    }

    analogWrite(LED_RED_DISTANCE_PIN, lightLevel);
    analogWrite(LED_GREEN_DISTANCE_PIN, lightLevel);
    analogWrite(LED_BLUE_DISTANCE_PIN, lightLevel);
  }
  else if (didDetectPresence) {
    digitalWrite(LED_RED_DISTANCE_PIN, LOW);
    digitalWrite(LED_GREEN_DISTANCE_PIN, HIGH);
    digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
  }
  else if (didDetectFarePresence) {
    digitalWrite(LED_RED_DISTANCE_PIN, HIGH);
    digitalWrite(LED_GREEN_DISTANCE_PIN, HIGH);
    digitalWrite(LED_BLUE_DISTANCE_PIN, LOW); 
  }
  else if (didPIRDetect) {
    digitalWrite(LED_RED_DISTANCE_PIN, HIGH);
    digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
    digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
  }
  else if (pirDetectionUntilTimeMs >  nowTimeMs) {
    digitalWrite(LED_RED_DISTANCE_PIN, LOW);
    digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
    digitalWrite(LED_BLUE_DISTANCE_PIN, HIGH);
  }
  else
  {
    digitalWrite(LED_RED_DISTANCE_PIN, LOW);
    digitalWrite(LED_GREEN_DISTANCE_PIN, LOW);
    digitalWrite(LED_BLUE_DISTANCE_PIN, LOW);
  }
}

void valveOn() {
  digitalWrite(VALVE_1_OFF_PIN, HIGH);
  digitalWrite(VALVE_1_ON_PIN, LOW);
  delay(SWITCH_DELAY_MS);
  digitalWrite(VALVE_1_ON_PIN, HIGH);

  isWatering = true;
}

void valveOff() {
  digitalWrite(VALVE_1_ON_PIN, HIGH);
  digitalWrite(VALVE_1_OFF_PIN, LOW);
  delay(SWITCH_DELAY_MS);
  digitalWrite(VALVE_1_OFF_PIN, HIGH);

  isWatering = false;
}

