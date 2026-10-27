#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main() {
    int sock;
    struct sockaddr_in sender, receiver;

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = INADDR_ANY;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    bind(sock, (struct sockaddr *)&receiver, sizeof(receiver));

    socklen_t len = sizeof(sender);
    int buffer, tt, tp, loss;

    recvfrom(sock, &tt, sizeof(tt), 0, (struct sockaddr *)&sender, &len);
    recvfrom(sock, &tp, sizeof(tp), 0, (struct sockaddr *)&sender, &len);
    recvfrom(sock, &loss, sizeof(loss), 0, (struct sockaddr *)&sender, &len);

    time_t start_time = time(NULL);
    int ack = 1;
    int success_count = 0;

    while (1) {
        recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *)&sender, &len);
        sleep(tp);

        printf("t%d: Received Packet %d from transmitter\n", (int)difftime(time(NULL), start_time), buffer);
        sleep(tt);

        int drop = 0;
        if (loss > 0 && success_count == loss - 1) {
            drop = 1;
            success_count = 0;
        }

        if (drop) {
            printf("t%d: ACK not sent for Packet %d (simulated loss)\n", (int)difftime(time(NULL), start_time), buffer);
        } else {
            sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&sender, len);
            printf("t%d: ACK sent for Packet %d\n", (int)difftime(time(NULL), start_time), buffer);
            success_count++;
        }
    }

    close(sock);
    return 0;
}
