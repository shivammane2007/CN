#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000
#define MAX_CHIPS 400
#define BARKER_LENGTH 11

const int barker[BARKER_LENGTH] = {
    1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0
};

unsigned int binaryToDecimal(int binary[], int size) {
    unsigned int decimal = 0;

    for (int i = 0; i < size; i++)
        decimal = decimal * 2 + binary[i];

    return decimal;
}

int despread(int receivedData[], int chips) {
    int binary[MAX_CHIPS / BARKER_LENGTH];
    int bitCount = 0;

    if (chips <= 0 || chips % BARKER_LENGTH != 0) {
        printf("Invalid number of received chips!\n");
        return 1;
    }

    for (int i = 0; i < chips; i += BARKER_LENGTH) {
        int matchOne = 1;
        int matchZero = 1;

        for (int j = 0; j < BARKER_LENGTH; j++) {
            if (receivedData[i + j] != barker[j])
                matchOne = 0;

            if (receivedData[i + j] != 1 - barker[j])
                matchZero = 0;
        }

        if (matchOne)
            binary[bitCount++] = 1;
        else if (matchZero)
            binary[bitCount++] = 0;
        else {
            printf("\nInvalid chip sequence detected!\n");
            return 1;
        }
    }

    printf("\nRecovered Binary: ");

    for (int i = 0; i < bitCount; i++)
        printf("%d", binary[i]);

    printf("\nOriginal Data: %u\n",
           binaryToDecimal(binary, bitCount));

    return 0;
}

int main() {
    int receiverSocket;
    int receivedData[MAX_CHIPS];

    struct sockaddr_in receiverAddress = {0};
    struct sockaddr_in transmitterAddress = {0};

    socklen_t len = sizeof(transmitterAddress);

    receiverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (receiverSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(PORT);
    receiverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(receiverSocket,
             (struct sockaddr *)&receiverAddress,
             sizeof(receiverAddress)) < 0) {
        perror("Bind failed");
        close(receiverSocket);
        return 1;
    }

    printf("CDMA Receiver waiting for data...\n");

    ssize_t bytes = recvfrom(
        receiverSocket,
        receivedData,
        sizeof(receivedData),
        0,
        (struct sockaddr *)&transmitterAddress,
        &len
    );

    if (bytes < 0) {
        perror("Receive failed");
        close(receiverSocket);
        return 1;
    }

    if (bytes == 0 || bytes % sizeof(int) != 0) {
        printf("Invalid received data!\n");
        close(receiverSocket);
        return 1;
    }

    int chips = bytes / sizeof(int);

    if (chips > MAX_CHIPS) {
        printf("Too many received chips!\n");
        close(receiverSocket);
        return 1;
    }

    printf("\nReceived Chips: ");

    for (int i = 0; i < chips; i++)
        printf("%d", receivedData[i]);

    printf("\n");

    despread(receivedData, chips);

    close(receiverSocket);
    return 0;
}