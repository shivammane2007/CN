#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_PACKETS 100

char *window_str(int base, int window_size, int total_packets) {
    static char buf[128];
    int end = (base + window_size - 1 > total_packets) ? total_packets : (base + window_size - 1);
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "[");
    for (int i = base; i <= end; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", i);
        if (i < end) pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
    }
    snprintf(buf + pos, sizeof(buf) - pos, "]");
    return buf;
}

int main() {
    int sock;
    struct sockaddr_in receiver;
    socklen_t len = sizeof(receiver);
    int total_packets, window_size, tt, tp, loss;

    printf("Enter number of packets       : ");
    scanf("%d", &total_packets);
    printf("Enter window size             : ");
    scanf("%d", &window_size);
    printf("Enter transmission time (Tt)  : ");
    scanf("%d", &tt);
    printf("Enter propagation time (Tp)   : ");
    scanf("%d", &tp);
    printf("Enter loss interval           : ");
    scanf("%d", &loss);

    if (total_packets <= 0 || total_packets > MAX_PACKETS || window_size <= 0 || window_size > total_packets || tt <= 0 || tp < 0) {
        printf("Invalid input.\n");
        return 1;
    }

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    sendto(sock, &total_packets, sizeof(total_packets), 0, (struct sockaddr *)&receiver, sizeof(receiver));
    sendto(sock, &loss, sizeof(loss), 0, (struct sockaddr *)&receiver, sizeof(receiver));

    int send_time[MAX_PACKETS + 1];
    int acknowledged[MAX_PACKETS + 1] = {0};
    int base = 1, next = 1;
    int current_time = tt;
    int total_transmissions = 0;

    struct timeval sock_timeout;
    sock_timeout.tv_sec = 1;
    sock_timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sock_timeout, sizeof(sock_timeout));

    printf("\nTransmitter Started  (N=%d, W=%d, Tt=%d, Tp=%d, Loss interval=%d)\n\n", total_packets, window_size, tt, tp, loss);

    while (base <= total_packets) {
        while (next < base + window_size && next <= total_packets) {
            send_time[next] = current_time;
            sendto(sock, &next, sizeof(next), 0, (struct sockaddr *)&receiver, sizeof(receiver));
            total_transmissions++;
            printf("%-14s At t=%-3d : Sent Packet %d\n", window_str(base, window_size, total_packets), current_time, next);
            next++;
            current_time += tt;
        }

        int ack_due = send_time[base] + 2 * tp;
        while (current_time < ack_due) {
            sleep(1);
            current_time++;
        }

        int ack;
        int result = recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&receiver, &len);

        if (result > 0) {
            acknowledged[ack] = 1;
            printf("%-14s At t=%-3d : ACK %d arrived (sent at t=%d, +2*Tp=%d)\n", window_str(base, window_size, total_packets), current_time, ack, send_time[ack], 2 * tp);

            while (base <= total_packets && acknowledged[base]) base++;

            while (next < base + window_size && next <= total_packets) {
                send_time[next] = current_time;
                sendto(sock, &next, sizeof(next), 0, (struct sockaddr *)&receiver, sizeof(receiver));
                total_transmissions++;
                printf("%-14s At t=%-3d : Window slid, sending Packet %d\n", window_str(base, window_size, total_packets), current_time, next);
                next++;
                current_time += tt;
            }
        } else {
            printf("%-14s At t=%-3d : Timeout waiting for ACK %d -> retransmitting window [%d..%d]\n", window_str(base, window_size, total_packets), current_time, base, base, next - 1);
            int resend_upto = next;
            next = base;
            while (next < resend_upto) {
                send_time[next] = current_time;
                sendto(sock, &next, sizeof(next), 0, (struct sockaddr *)&receiver, sizeof(receiver));
                total_transmissions++;
                printf("%-14s At t=%-3d : Retransmitted Packet %d\n", window_str(base, window_size, total_packets), current_time, next);
                next++;
                current_time += tt;
            }
        }
    }

    printf("\nAll packets acknowledged.\n");
    printf("Total transmissions  : %d\n", total_transmissions);

    close(sock);
    return 0;
}
