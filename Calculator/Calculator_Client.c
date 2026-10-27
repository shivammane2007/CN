#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct Data {
    int num1;
    int num2;
    char op;
};

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr = {0};
    struct Data d;
    int result, stop = 1;
    socklen_t len = sizeof(serverAddr);

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (clientSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    while (1) {
        printf("\nEnter first number: ");
        if (scanf("%d", &d.num1) != 1)
            break;

        printf("Enter operator (+ - * /): ");
        if (scanf(" %c", &d.op) != 1)
            break;

        printf("Enter second number: ");
        if (scanf("%d", &d.num2) != 1)
            break;

        if (d.op != '+' && d.op != '-' &&
            d.op != '*' && d.op != '/') {
            printf("Invalid operator!\n");
            continue;
        }

        if (d.op == '/' && d.num2 == 0) {
            printf("Cannot divide by zero!\n");
            continue;
        }

        if (sendto(clientSocket, &d, sizeof(d), 0,
                   (struct sockaddr *)&serverAddr,
                   sizeof(serverAddr)) < 0) {
            perror("Send failed");
            break;
        }

        if (recvfrom(clientSocket, &result, sizeof(result), 0,
                     (struct sockaddr *)&serverAddr, &len) < 0) {
            perror("Receive failed");
            break;
        }

        printf("Result = %d\n", result);

        printf("Enter 1 to continue, 0 to stop: ");
        if (scanf("%d", &stop) != 1)
            break;

        sendto(clientSocket, &stop, sizeof(stop), 0,
               (struct sockaddr *)&serverAddr,
               sizeof(serverAddr));

        if (stop == 0)
            break;
    }

    close(clientSocket);
    return 0;
}