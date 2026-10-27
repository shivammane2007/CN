#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>

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

    int maximum_packets, window_size, transmission_time, propagation_time;
    int number_of_sequence_bits, sequence_number_range;
    
    int loss = 0;

    printf("Enter Transmission Time (Tt): ");
    scanf("%d", &transmission_time);

    printf("Enter Propagation Time (Tp): ");
    scanf("%d", &propagation_time);
    
    printf("Enter loss interval: ");
    scanf("%d", &loss);

    if (transmission_time <= 0 || propagation_time < 0) {
        printf("Invalid input.\n");
        return 1;
    }

    float a = (float)propagation_time / transmission_time;
    maximum_packets = 1 + (int)(2 * a);
    number_of_sequence_bits = (int)floor(log2(maximum_packets));
    sequence_number_range = (int)pow(2, number_of_sequence_bits);
    window_size = (sequence_number_range < maximum_packets) ? sequence_number_range : maximum_packets;

    int total_packets = maximum_packets;

    if (total_packets > MAX_PACKETS) {
        printf("Calculated number of packets exceeds MAX_PACKETS.\n");
        return 1;
    }

    printf("\n========================================\n");
    printf("       SELECTIVE REPEAT TRANSMITTER\n");
    printf("========================================\n");
    printf("Transmission Time (Tt) : %d\n", transmission_time);
    printf("Propagation Time (Tp)  : %d\n", propagation_time);
    printf("Maximum Packets        : %d\n", maximum_packets);
    printf("Sequence Bits          : %d\n", number_of_sequence_bits);
    printf("Sequence Number Range  : %d\n", sequence_number_range);
    printf("Window Size            : %d\n", window_size);
    printf("========================================\n");

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
    int current_time = transmission_time;
    int total_transmissions = 0;

    struct timeval sock_timeout;
    sock_timeout.tv_sec = 1;
    sock_timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sock_timeout, sizeof(sock_timeout));

    printf("\nTransmitter Started  (N=%d, W=%d, Tt=%d, Tp=%d)\n\n", total_packets, window_size, transmission_time, propagation_time);

    while (base <= total_packets) {
        while (next < base + window_size && next <= total_packets) {
            send_time[next] = current_time;
            sendto(sock, &next, sizeof(next), 0, (struct sockaddr *)&receiver, sizeof(receiver));
            total_transmissions++;
            printf("%-14s At t=%-3d : Sent Packet %d\n", window_str(base, window_size, total_packets), current_time, next);
            next++;
            current_time += transmission_time;
        }

        int ack_due = send_time[base] + 2 * propagation_time;
        while (current_time < ack_due) {
            sleep(1);
            current_time++;
        }

        int ack;
        int result = recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&receiver, &len);

        if (result > 0) {
            acknowledged[ack] = 1;
            printf("%-14s At t=%-3d : ACK %d arrived (sent at t=%d, +2*Tp=%d)\n", window_str(base, window_size, total_packets), current_time, ack, send_time[ack], 2 * propagation_time);

            while (base <= total_packets && acknowledged[base]) {
                base++;
            }

            while (next < base + window_size && next <= total_packets) {
                send_time[next] = current_time;
                sendto(sock, &next, sizeof(next), 0, (struct sockaddr *)&receiver, sizeof(receiver));
                total_transmissions++;
                printf("%-14s At t=%-3d : Window slid, sending Packet %d\n", window_str(base, window_size, total_packets), current_time, next);
                next++;
                current_time += transmission_time;
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
                current_time += transmission_time;
            }
        }
    }

    printf("\nAll packets acknowledged.\n");
    printf("Total transmissions  : %d\n", total_transmissions);

    close(sock);
    return 0;
}
