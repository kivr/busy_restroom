#include <SPI.h>
#include "RF24.h"
#include "LowPower.h"

#define DOOR_PIN 2
#define PIR_PIN 3
#define ULTRA_POWER_PIN 5

#define PAYLOAD_SIZE 1

/*************  USER Configuration *****************************/
                                           // Hardware configuration
RF24 radio(15, 14);                           // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

/***************************************************************/

const uint64_t pipe = 0xABCDABCD71LL;   // Radio pipe addresses for the 2 nodes to communicate.

unsigned long rxTimer;            //Counter and timer for keeping track transfer info
unsigned long startTime, stopTime;

byte prevDoorValue = LOW;

void wakeUp() {

}

void sleep() {
  Serial.flush();

  byte doorValue = digitalRead(DOOR_PIN);

  // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(0, wakeUp, doorValue == HIGH ? FALLING : RISING);
  attachInterrupt(1, wakeUp, RISING);
  
  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
  
  // Disable external pin interrupt on wake up pin.
  detachInterrupt(0);
  detachInterrupt(1);
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
  pinMode(PIR_PIN, INPUT);
  digitalWrite(PIR_PIN, LOW);
  pinMode(ULTRA_POWER_PIN, OUTPUT);
  digitalWrite(ULTRA_POWER_PIN, LOW);

  initRadio();
}

void sendData(char data) {
  Serial.println(F("Initiating Basic Data Transfer"));
  
  radio.powerUp();
          
  radio.writeFast(&data, PAYLOAD_SIZE);
                                       //This should be called to wait for completion and put the radio in standby mode after transmission, returns 0 if data still in FIFO (timed out), 1 if success
  bool txOK = radio.txStandBy(3000);
  
  Serial.print("Transfer complete ");
  Serial.print(txOK ? "OK" : "ERROR");
  Serial.println();

  radio.powerDown();
}

void loop(void){

  delay(20);
  byte doorValue = digitalRead(DOOR_PIN);
  byte pirValue = digitalRead(PIR_PIN);

  if (doorValue != prevDoorValue) {
    sendData(doorValue == HIGH ? 'O' : 'C');
    prevDoorValue = doorValue;
  } else if (pirValue == HIGH) {
    sendData('P');
  }

  sleep();
}
