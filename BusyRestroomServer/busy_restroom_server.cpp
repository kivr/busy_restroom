#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <RF24/RF24.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_CLIENT_COUNT 10
#define SOCK_PATH "/var/run/busy_restroom"

using namespace std;
//
// Hardware configuration
//

/****************** Raspberry Pi ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939 and the related enumerations for pin information.

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

/*** RPi Alternate ***/
//Note: Specify SPI BUS 0 or 1 instead of CS pin number.
// See http://tmrh20.github.io/RF24/RPi.html for more information on usage

//RPi Alternate, with MRAA
//RF24 radio(15,0);

//RPi Alternate, with SPIDEV - Note: Edit RF24/arch/BBB/spi.cpp and  set 'this->device = "/dev/spidev0.0";;' or as listed in /dev
//RF24 radio(22,0);


/****************** Linux (BBB,x86,etc) ***********************/

// See http://tmrh20.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Setup for ARM(Linux) devices like BBB using spidev (default is "/dev/spidev1.0" )
//RF24 radio(115,0);

//BBB Alternate, with mraa
// CE pin = (Header P9, Pin 13) = 59 = 13 + 46 
//Note: Specify SPI BUS 0 or 1 instead of CS pin number. 
//RF24 radio(59,0);

/**************************************************************/

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t address = 0xABCDABCD71LL;


uint8_t data[1];
unsigned long startTime, stopTime, counter, rxTimer=0;
int clients[MAX_CLIENT_COUNT];
int client_count = 0;

double mean(uint8_t *data, int length) {
    double sum = 0;
    int i;
    int count = 0;

    for (i = 0; i < length; i++) {
        if (data[i] != 255) {
            sum += data[i];
            count++;
        }
    }

    return sum / count;
}

double variance(uint8_t *data, int length) {
    double ssum = 0;
    double m = mean(data, length);
    int i;
    int count = 0;

    for (i = 0; i < length; i++) {
        if (data[i] != 255) {
            double diff = data[i] - m;
            ssum += diff * diff;
            count++;
        }
    }

    return ssum / count;
}

void *startServerThread(void *ptr) {
    int server_socket, client_socket;
    sockaddr_un addr;

    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);

    unlink(SOCK_PATH);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);
    bind(server_socket, (sockaddr*)&addr, sizeof(addr));
    listen(server_socket, 10);
   
    while (client_count < MAX_CLIENT_COUNT) {
        clients[client_count] = accept(server_socket, NULL, 0);
        if (clients[client_count] != -1) {
            printf("Client %d\n", clients[client_count]);
            client_count++;
        }
    }
}

int main(int argc, char** argv){
    pthread_t serverThread;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    pthread_create(&serverThread, NULL, startServerThread, NULL);

    // Print preamble:

    radio.begin();                           // Setup and configure rf radio
    radio.setChannel(1);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setAutoAck(1);                     // Ensure autoACK is enabled
    radio.setRetries(2,15);                  // Optionally, increase the delay between retries & # of retries
    radio.setPayloadSize(1);
    radio.setCRCLength(RF24_CRC_8);          // Use 8-bit CRC for performance
    radio.printDetails();
    /********* Role chooser ***********/

    radio.openReadingPipe(1,address);
    radio.startListening();

    // forever loop
    while (1){

        while(radio.available()){
            radio.read(&data,8);
            counter++;
        }

        if(counter > 0){
            int i;

            rxTimer = millis();
            float numBytes = counter*32;
            printf("%s", data[0] == 'C' ? "Closed" : data[0] == 'P' ? "Person" : "Opened");

//            if (data[0] == 'C') {
//                int i = 0;
//                double m = mean(data + 1, 7);
//                double v = variance(data + 1, 7);
//                printf("m:%.2f v:%.2f r:%c\n", m, v,
//                        m > 190.0 && v < 1000 ? 'E' : 'F');
//                for (i = 1; i < 8; i++) {
//                    printf("%d ", data[i]);
//                }
//            }
            printf("\n");

            // Notify clients
            for (i = 0; i < client_count; i++) {
                write(clients[i], &data, 1);
            }

            counter = 0;
        }
    }

} // main
