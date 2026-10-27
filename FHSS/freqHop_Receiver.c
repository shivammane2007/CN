#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_COUNT 5
#define MAX_MESSAGES 100
#define MAX_LENGTH 256

int main() {
    int port[PORT_COUNT] = {9000, 9001, 9002, 9003, 9004};
    int hop[] = {3, 1, 0, 4, 2};

    int receiverSocket[PORT_COUNT];
    struct sockaddr_in receiverAddress[PORT_COUNT] = {0};
    struct sockaddr_in transmitterAddress;

    int totalHop = sizeof(hop) / sizeof(hop[0]);
    int totalMsg;

    printf("Enter number of messages to receive: ");

    if (scanf("%d", &totalMsg) != 1 ||
        totalMsg < 1 || totalMsg > MAX_MESSAGES) {
        printf("Invalid number of messages!\n");
        return 1;
    }

    // Create and bind sockets to all ports
    for (int i = 0; i < PORT_COUNT; i++) {
        receiverSocket[i] = socket(AF_INET, SOCK_DGRAM, 0);

        if (receiverSocket[i] < 0) {
            perror("Socket creation failed");
            return 1;
        }

        receiverAddress[i].sin_family = AF_INET;
        receiverAddress[i].sin_port = htons(port[i]);
        receiverAddress[i].sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(receiverSocket[i],
                 (struct sockaddr *)&receiverAddress[i],
                 sizeof(receiverAddress[i])) < 0) {
            perror("Bind failed");

            for (int j = 0; j <= i; j++)
                close(receiverSocket[j]);

            return 1;
        }
    }

    // Clear the input buffer
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);

    printf("\nFHSS Receiver waiting for %d messages...\n\n",
           totalMsg);

    for (int i = 0; i < totalMsg; i++) {
        char buffer[MAX_LENGTH] = {0};
        socklen_t len = sizeof(transmitterAddress);

        int channel = hop[i % totalHop];

        int bytes = recvfrom(
            receiverSocket[channel],
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&transmitterAddress,
            &len
        );

        if (bytes < 0) {
            perror("Receive failed");
            break;
        }

        buffer[bytes] = '\0';

        printf("Received: %s | Port: %d\n",
               buffer, port[channel]);
    }

    for (int i = 0; i < PORT_COUNT; i++)
        close(receiverSocket[i]);

    return 0;
}