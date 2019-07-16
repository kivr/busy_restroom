#include <SPI.h>
#include "RF24.h"
#include <avr/sleep.h>

#define DOOR_PIN 2
#define ULTRA_POWER_PIN 5
#define PAYLOAD_SIZE 1

volatile byte personDetected = 0;

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

void setPersonDetected() {
  personDetected = 1;
  detachInterrupt(1);
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

void initRadio() {
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
}

void setup(void) {

  Serial.begin(115200);

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(ULTRA_POWER_PIN, OUTPUT);
  digitalWrite(ULTRA_POWER_PIN, LOW);

  initRadio();
}

void loop(void){
  counter = 0;

  if (personDetected) {
    data[0] = 'P';
    personDetected = 0;
  } else {
    detachInterrupt(1);
    delay(20);
    byte value = digitalRead(DOOR_PIN);
  
    data[0] = value == HIGH ? 'O' : 'C';
  
    digitalWrite(ULTRA_POWER_PIN, HIGH);
  
    if (value == LOW) {
      attachInterrupt(1, setPersonDetected, RISING);
    }
  }
  
  Serial.println(F("Initiating Basic Data Transfer"));

  radio.powerUp();
          
  if(!radio.writeFast(&data, PAYLOAD_SIZE)){   //Write to the FIFO buffers        
    counter++;                      //Keep count of failed payloads
  }
                                       //This should be called to wait for completion and put the radio in standby mode after transmission, returns 0 if data still in FIFO (timed out), 1 if success
  bool txOK = radio.txStandBy(3000);
  
  Serial.print("Transfer complete ");
  Serial.print(txOK ? "OK" : "ERROR");
  Serial.println();

  radio.powerDown();

  digitalWrite(ULTRA_POWER_PIN, LOW);

  if (!personDetected) {
    sleep();
  }
}
