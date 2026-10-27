#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_PACKETS 100

int main() {
    int sock;
    struct sockaddr_in receiver, transmitter;
    socklen_t len = sizeof(transmitter);

    int total_packets, loss;
    int expected = 1;
    int count = 0;
    int lost[MAX_PACKETS + 1] = {0};

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = INADDR_ANY;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    bind(sock, (struct sockaddr *)&receiver, sizeof(receiver));

    recvfrom(sock, &total_packets, sizeof(total_packets), 0, (struct sockaddr *)&transmitter, &len);
    recvfrom(sock, &loss, sizeof(loss), 0, (struct sockaddr *)&transmitter, &len);

    printf("Receiver Started  (N=%d, Loss interval=%d)\n\n", total_packets, loss);
    printf("Rule: after every %d successful ACK(s), the next packet is dropped once.\n\n", (loss > 0) ? loss - 1 : 0);

    while (expected <= total_packets) {
        int packet;
        recvfrom(sock, &packet, sizeof(packet), 0, (struct sockaddr *)&transmitter, &len);

        if (packet == expected) {
            if (lost[packet]) {
                printf("Packet %d received (recovered) -> ", packet);
                sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr *)&transmitter, len);
                printf("ACK %d sent\n\n", packet);
                expected++;
                count = 1;
            } else if (count == loss - 1) {
                lost[packet] = 1;
                printf("Packet %d simulated LOST (no ACK sent)\n\n", packet);
                continue;
            } else {
                printf("Packet %d received -> ", packet);
                sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr *)&transmitter, len);
                printf("ACK %d sent\n\n", packet);
                expected++;
                count++;
            }
        } else {
            printf("Packet %d rejected (expected %d)\n\n", packet, expected);
        }
    }

    printf("All packets received successfully.\n");
    close(sock);
    return 0;
}
