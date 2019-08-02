#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LIGHT_BULB_PIN 7

int main (void)
{
    int sock;
    int result;
    struct sockaddr_un address;

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "/var/run/busy_restroom");

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    result = connect(sock, (struct sockaddr*)&address, sizeof(address));
    if (result < 0)
        return 1;

    if (wiringPiSetup () == -1)
        return 1;

    pinMode (LIGHT_BULB_PIN, OUTPUT) ;         // aka BCM_GPIO pin 17

    for (;;)
    {
        char data;
        ssize_t result = recv(sock, &data, 1, 0);

        if (result == 1) {
            if (data == 'C') {
                printf("Closed\n");
                digitalWrite (LIGHT_BULB_PIN, 1) ;       // On
            } else if (data == 'O') {
                printf("Opened\n");
                digitalWrite (LIGHT_BULB_PIN, 0) ;       // Off
            }
        } else {
            printf("Error\n");
            break;
        }
    }
    return 0 ;
}
