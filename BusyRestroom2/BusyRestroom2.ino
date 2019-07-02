#include <SPI.h>
#include "RF24.h"
#include <avr/sleep.h>
#include <HCSR04.h>

#define DOOR_PIN 2
#define ULTRA_POWER_PIN 5
#define ULTRA_TRIGGER_PIN 4
#define ULTRA_ECHO_PIN 3
#define PAYLOAD_SIZE 8

/*************  USER Configuration *****************************/
                                           // Hardware configuration
RF24 radio(15, 14);                           // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

/***************************************************************/

const uint64_t pipe = 0xABCDABCD71LL;   // Radio pipe addresses for the 2 nodes to communicate.

byte data[PAYLOAD_SIZE];                             //Data buffer for testing data transfer speeds

unsigned long counter, rxTimer;            //Counter and timer for keeping track transfer info
unsigned long startTime, stopTime;

void wakeUp() {
  sleep_disable();
  detachInterrupt(0);
}

void sleep() {
  Serial.flush();
  
  int value = digitalRead(DOOR_PIN);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  noInterrupts();
  attachInterrupt(0, wakeUp, value == HIGH ? LOW : HIGH);
  interrupts();
  sleep_cpu();
}

double readDistance() {
  double cm;
  int retries = 0;
  long duration;
  
  // Reset the trigger pin and get ready for a clean trigger pulse
  digitalWrite(ULTRA_TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRIGGER_PIN, HIGH); // You need to keep it high for 10 micro seconds length
  delayMicroseconds(10); // This is the 10 microseconds we mentioned above :)
  digitalWrite(ULTRA_TRIGGER_PIN, LOW); // Stop the trigger pulse after the 10 microseconds

  duration = pulseIn(ULTRA_ECHO_PIN, HIGH, 20000);
  
  while (duration == 0 && retries < 5) {
    Serial.print("Unstucking ultrasonic sensor...\n");
    pinMode(ULTRA_ECHO_PIN, OUTPUT);
    digitalWrite(ULTRA_ECHO_PIN, LOW);
    delayMicroseconds(10);
    pinMode(ULTRA_ECHO_PIN, INPUT_PULLUP);
    
    retries++;
    duration = pulseIn(ULTRA_ECHO_PIN, HIGH, 20000);
  }
  
  cm = (duration/2.0) / 29.1;

  return cm;
}

void setup(void) {

  Serial.begin(115200);

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(ULTRA_POWER_PIN, OUTPUT);
  pinMode(ULTRA_TRIGGER_PIN, OUTPUT);
  pinMode(ULTRA_ECHO_PIN, INPUT_PULLUP);
  digitalWrite(ULTRA_POWER_PIN, LOW);

  radio.begin();                           // Setup and configure rf radio
  radio.setChannel(1);
  radio.setPALevel(RF24_PA_MAX);           // If you want to save power use "RF24_PA_MIN" but keep in mind that reduces the module's range
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(1);                     // Ensure autoACK is enabled
  radio.setRetries(15,15);                  // Optionally, increase the delay between retries & # of retries
  radio.setPayloadSize(PAYLOAD_SIZE);
  radio.setCRCLength(RF24_CRC_8);          // Use 8-bit CRC for performance
  radio.openWritingPipe(pipe);
  
  radio.stopListening();                  // Start listening
  radio.printDetails();                    // Dump the configuration of the rf unit for debugging
  
  Serial.println(F("\n\rRF24/examples/Transfer/"));
  
  for(int i = 0; i < PAYLOAD_SIZE; i++){
     data[i] = 'K';
  }
}

void loop(void){
  counter = 0;
  
  delay(20);
  byte value = digitalRead(DOOR_PIN);

  data[0] = value == HIGH ? 'O' : 'C';

  if (value == LOW) {
    int i;

    digitalWrite(ULTRA_POWER_PIN, HIGH);
    
    for (i = 1; i < PAYLOAD_SIZE; i++) {
      delay(60);
      data[i] = (unsigned byte)readDistance();
      Serial.print(data[i]);
      Serial.println();
    }

    digitalWrite(ULTRA_POWER_PIN, LOW);
  }
  
  Serial.println(F("Initiating Basic Data Transfer"));

  radio.powerUp();
          
  if(!radio.writeFast(&data,32)){   //Write to the FIFO buffers        
    counter++;                      //Keep count of failed payloads
  }
                                       //This should be called to wait for completion and put the radio in standby mode after transmission, returns 0 if data still in FIFO (timed out), 1 if success
  bool txOK = radio.txStandBy(10000);
  
  Serial.print("Transfer complete ");
  Serial.print(txOK ? "OK" : "ERROR");
  Serial.println();

  radio.powerDown();

  sleep();
}