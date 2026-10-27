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

    int transmitterSocket[PORT_COUNT];
    struct sockaddr_in receiverAddress[PORT_COUNT] = {0};

    int totalHop = sizeof(hop) / sizeof(hop[0]);
    int totalMsg;

    printf("Enter number of messages: ");

    if (scanf("%d", &totalMsg) != 1 ||
        totalMsg < 1 || totalMsg > MAX_MESSAGES) {
        printf("Invalid number of messages!\n");
        return 1;
    }

    // Clear the input buffer
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);

    // Create sockets for all hopping channels
    for (int i = 0; i < PORT_COUNT; i++) {
        transmitterSocket[i] = socket(AF_INET, SOCK_DGRAM, 0);

        if (transmitterSocket[i] < 0) {
            perror("Socket creation failed");
            return 1;
        }

        receiverAddress[i].sin_family = AF_INET;
        receiverAddress[i].sin_port = htons(port[i]);
        receiverAddress[i].sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    printf("\nFHSS Transmitter Started\n");

    for (int i = 0; i < totalMsg; i++) {
        char message[MAX_LENGTH];

        printf("Enter message %d: ", i + 1);

        if (fgets(message, sizeof(message), stdin) == NULL)
            break;

        message[strcspn(message, "\n")] = '\0';

        int channel = hop[i % totalHop];

        int sent = sendto(
            transmitterSocket[channel],
            message,
            strlen(message) + 1,
            0,
            (struct sockaddr *)&receiverAddress[channel],
            sizeof(receiverAddress[channel])
        );

        if (sent < 0) {
            perror("Send failed");
            break;
        }

        printf("Sent: %s | Port: %d\n", message, port[channel]);

        sleep(1);
    }

    for (int i = 0; i < PORT_COUNT; i++)
        close(transmitterSocket[i]);

    return 0;
}