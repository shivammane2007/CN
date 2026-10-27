#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int serversocket;
    char servermessage[] = "Hello Client and we are connected now";
    char buffer[256];

    struct sockaddr_in serveraddress, clientaddress;
    socklen_t len = sizeof(clientaddress);

    // Create UDP socket
    serversocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (serversocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    printf("Server socket created successfully\n");

    // Configure server address
    memset(&serveraddress, 0, sizeof(serveraddress));

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(9000);
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket
    if (bind(serversocket, (struct sockaddr *)&serveraddress,
             sizeof(serveraddress)) < 0) {
        perror("Binding failed");
        close(serversocket);
        return 1;
    }

    printf("Binding successful\n");
    printf("Waiting for client message...\n");

    // Receive message from client
    int n = recvfrom(serversocket, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&clientaddress, &len);

    if (n < 0) {
        perror("Receiving failed");
        close(serversocket);
        return 1;
    }

    buffer[n] = '\0';

    printf("Client Message: %s\n", buffer);

    // Send reply to client
    if (sendto(serversocket, servermessage, strlen(servermessage) + 1, 0,
               (struct sockaddr *)&clientaddress, len) < 0) {
        perror("Sending reply failed");
        close(serversocket);
        return 1;
    }

    printf("Reply sent to client successfully\n");

    close(serversocket);

    return 0;
}