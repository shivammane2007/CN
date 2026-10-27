#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int clientsocket;
    char clientmessage[] = "Hello server";
    char serverresponse[256];

    struct sockaddr_in serveraddress;
    socklen_t len = sizeof(serveraddress);

    // Create UDP socket
    clientsocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (clientsocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    printf("Client socket created successfully\n");

    // Configure server address
    memset(&serveraddress, 0, sizeof(serveraddress));

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(9000);

    // Send to the server on the same machine
    serveraddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Send message to server
    if (sendto(clientsocket, clientmessage, strlen(clientmessage) + 1, 0,
               (struct sockaddr *)&serveraddress,
               sizeof(serveraddress)) < 0) {
        perror("Sending failed");
        close(clientsocket);
        return 1;
    }

    printf("Message sent to server: %s\n", clientmessage);

    // Receive reply from server
    int n = recvfrom(clientsocket, serverresponse,
                     sizeof(serverresponse) - 1, 0,
                     (struct sockaddr *)&serveraddress, &len);

    if (n < 0) {
        perror("Receiving reply failed");
        close(clientsocket);
        return 1;
    }

    serverresponse[n] = '\0';

    printf("Reply from server: %s\n", serverresponse);

    close(clientsocket);

    return 0;
}
