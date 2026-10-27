#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct setup {
    int transmission_time;
    int propagation_time;
    int maximum_packets;
    int number_of_sequence_bits;
    int sequence_number_range;
    int window_size;
};

int main() {
    int socket_number;

    struct sockaddr_in receiver;
    struct sockaddr_in transmitter;

    int packet;
    int acknowledgement;

    struct setup information;

    socklen_t address_length = sizeof(transmitter);

    socket_number = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_number < 0) {
        perror("Socket creation failed");
        return 1;
    }

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_number, (struct sockaddr *)&receiver, sizeof(receiver)) < 0) {
        perror("Bind failed");
        close(socket_number);
        return 1;
    }

    printf("Receiver Started...\n");
    printf("Waiting for setup information...\n\n");

    recvfrom(socket_number, &information, sizeof(information), 0, (struct sockaddr *)&transmitter, &address_length);

    printf("Setup Information Received\n");

    printf("Transmission Time       : %d ms\n", information.transmission_time);

    printf("Propagation Time        : %d ms\n", information.propagation_time);

    printf("Maximum Packets         : %d\n", information.maximum_packets);

    printf("Sequence Bits           : %d\n", information.number_of_sequence_bits);

    printf("Sequence Number Range   : %d\n", information.sequence_number_range);

    printf("Window Size             : %d\n\n", information.window_size);

    printf("Waiting for packets...\n\n");

    while (1) {
        recvfrom(socket_number, &packet, sizeof(packet), 0, (struct sockaddr *)&transmitter, &address_length);

        printf("Received Packet: %d\n", packet);

        usleep(information.propagation_time * 1000000);

        acknowledgement = packet;

        sendto(socket_number, &acknowledgement, sizeof(acknowledgement), 0, (struct sockaddr *)&transmitter, address_length);

        printf("ACK Sent: %d\n\n", acknowledgement);
    }

    close(socket_number);

    return 0;
}
