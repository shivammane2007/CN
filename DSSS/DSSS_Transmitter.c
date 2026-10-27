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

int decimalToBinary(unsigned int n, int binary[], int size) {
    int count = 0;

    if (n == 0) {
        binary[count++] = 0;
        return count;
    }

    while (n > 0 && count < size) {
        binary[count++] = n % 2;
        n /= 2;
    }

    for (int i = 0; i < count / 2; i++) {
        int temp = binary[i];
        binary[i] = binary[count - 1 - i];
        binary[count - 1 - i] = temp;
    }

    return count;
}

int main() {
    unsigned long long input;

    int binary[sizeof(unsigned int) * CHAR_BIT];
    int spreadData[MAX_CHIPS];
    int binarySize, k = 0;

    printf("Enter decimal data: ");

    if (scanf("%llu", &input) != 1 || input > UINT_MAX) {
        printf("Invalid input!\n");
        return 1;
    }

    unsigned int data = (unsigned int)input;

    binarySize = decimalToBinary(
        data, binary, sizeof(binary) / sizeof(binary[0])
    );

    for (int i = 0; i < binarySize; i++) {
        for (int j = 0; j < BARKER_LENGTH; j++) {
            if (binary[i] == 1)
                spreadData[k++] = barker[j];
            else
                spreadData[k++] = 1 - barker[j];
        }
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in receiverAddress = {0};

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(PORT);
    receiverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (sendto(sock, spreadData, k * sizeof(int), 0,
               (struct sockaddr *)&receiverAddress,
               sizeof(receiverAddress)) < 0) {
        perror("Send failed");
        close(sock);
        return 1;
    }

    printf("\nOriginal Data: %u\n", data);

    printf("Binary: ");
    for (int i = 0; i < binarySize; i++)
        printf("%d", binary[i]);

    printf("\nSpread Data: ");
    for (int i = 0; i < k; i++)
        printf("%d", spreadData[i]);

    printf("\n");

    close(sock);
    return 0;
}
