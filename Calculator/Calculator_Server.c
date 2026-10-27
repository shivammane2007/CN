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
    int serverSocket;
    struct sockaddr_in serverAddr = {0}, clientAddr = {0};
    struct Data d;
    int result, stop = 1;
    socklen_t len = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (serverSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    printf("Server is waiting...\n");

    do {
        len = sizeof(clientAddr);

        if (recvfrom(serverSocket, &d, sizeof(d), 0,
                     (struct sockaddr *)&clientAddr, &len) < 0) {
            perror("Receive failed");
            break;
        }

        switch (d.op) {
            case '+':
                result = d.num1 + d.num2;
                break;

            case '-':
                result = d.num1 - d.num2;
                break;

            case '*':
                result = d.num1 * d.num2;
                break;

            case '/':
                if (d.num2 == 0) {
                    printf("Cannot divide by zero!\n");
                    result = 0;
                } else {
                    result = d.num1 / d.num2;
                }
                break;

            default:
                result = 0;
        }

        sendto(serverSocket, &result, sizeof(result), 0,
               (struct sockaddr *)&clientAddr, len);

        printf("Result Sent = %d\n", result);

        if (recvfrom(serverSocket, &stop, sizeof(stop), 0,
                     (struct sockaddr *)&clientAddr, &len) < 0) {
            perror("Stop message receive failed");
            break;
        }

    } while (stop != 0);

    close(serverSocket);
    return 0;
}