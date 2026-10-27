#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

int min(int first, int second) {
    return (first < second) ? first : second;
}

struct setup {
    int transmission_time;
    int propagation_time;
    int maximum_packets;
    int number_of_sequence_bits;
    int sequence_number_range;
    int window_size;
};

void print_window(int first_packet, int next_packet, int sequence_number_range) {
    printf("[");
    for (int i = first_packet; i < next_packet; i++) {
        printf("%d", i % sequence_number_range);
        if (i < next_packet - 1) printf(" ");
    }
    printf("]");
}

void calculate_cycles(int first_packet, int next_packet, int window_size, int *c1, int *c2) {
    *c1 = 0;
    *c2 = 0;
    for (int i = first_packet; i < next_packet; i++) {
        if (i < window_size) (*c1)++;
        else (*c2)++;
    }
}

void print_state(int current_time, int acknowledgements_received, int c1, int c2, int T, int first_packet, int next_packet, int sequence_number_range) {
    printf("%8d %3d %3d %3d %3d   ", current_time, acknowledgements_received, c1, c2, T);
    print_window(first_packet, next_packet, sequence_number_range);
    printf("\n");
    fflush(stdout);
}

int main() {
    int socket_number;
    struct sockaddr_in receiver;

    int transmission_time, propagation_time;
    int maximum_packets, number_of_sequence_bits, sequence_number_range, window_size;

    int first_packet = 0, next_packet = 0;
    int packet, acknowledgement;
    int total_transmitted = 0, acknowledgements_received = 0;
    int current_time = 0;
    int c1 = 0, c2 = 0, T = 0;

    struct setup information;
    socklen_t address_length = sizeof(receiver);

    printf("Enter Transmission Time (seconds): ");
    scanf("%d", &transmission_time);

    printf("Enter Propagation Time (seconds): ");
    scanf("%d", &propagation_time);

    if (transmission_time <= 0 || propagation_time < 0) {
        printf("Invalid input.\n");
        return 1;
    }

    float a = (float)propagation_time / transmission_time;
    maximum_packets = 1 + (int)(2 * a);
    number_of_sequence_bits = (int)floor(log2(maximum_packets));
    sequence_number_range = (int)pow(2, number_of_sequence_bits);
    window_size = min(sequence_number_range, maximum_packets);

    printf("\nMaximum Packets        : %d\n", maximum_packets);
    printf("Sequence Bits          : %d\n", number_of_sequence_bits);
    printf("Sequence Number Range  : %d\n", sequence_number_range);
    printf("Window Size             : %d\n\n", window_size);

    socket_number = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_number < 0) {
        perror("Socket creation failed");
        return 1;
    }

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    information.transmission_time = transmission_time;
    information.propagation_time = propagation_time;
    information.maximum_packets = maximum_packets;
    information.number_of_sequence_bits = number_of_sequence_bits;
    information.sequence_number_range = sequence_number_range;
    information.window_size = window_size;

    sendto(socket_number, &information, sizeof(information), 0, (struct sockaddr *)&receiver, sizeof(receiver));

    printf("Setup information sent.\n\n");
    printf("Transmitter Started...\n\n");
    printf(" Time(s)   A   C1   C2   T   Current Window\n");

    print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);

    current_time = transmission_time;

    int *send_times = malloc(maximum_packets * sizeof(int));
    if (send_times == NULL) {
        perror("Memory allocation failed");
        close(socket_number);
        return 1;
    }

    while (acknowledgements_received < maximum_packets) {
        while (next_packet < first_packet + window_size && next_packet < maximum_packets) {
            send_times[next_packet] = current_time;
            packet = next_packet % sequence_number_range;
            sendto(socket_number, &packet, sizeof(packet), 0, (struct sockaddr *)&receiver, sizeof(receiver));

            next_packet++;
            total_transmitted++;

            calculate_cycles(first_packet, next_packet, window_size, &c1, &c2);
            T = c1 + c2;

            print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);

            sleep(transmission_time);
            current_time += transmission_time;

            if (next_packet >= first_packet + window_size || next_packet >= maximum_packets) {
                int ack_arrival_time = send_times[first_packet] + 2 * propagation_time;
                if (current_time < ack_arrival_time) {
                    print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);
                }
            }
        }

        int ack_arrival_time = send_times[first_packet] + 2 * propagation_time;

        while (current_time < ack_arrival_time) {
            sleep(1);
            current_time++;

            if (current_time < ack_arrival_time) {
                print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);
            }
        }

        recvfrom(socket_number, &acknowledgement, sizeof(acknowledgement), 0, (struct sockaddr *)&receiver, &address_length);

        if (acknowledgement == first_packet % sequence_number_range) {
            acknowledgements_received++;
            first_packet++;

            calculate_cycles(first_packet, next_packet, window_size, &c1, &c2);
            T = c1 + c2;

            if (next_packet < first_packet + window_size && next_packet < maximum_packets) {
                send_times[next_packet] = current_time;
                packet = next_packet % sequence_number_range;
                sendto(socket_number, &packet, sizeof(packet), 0, (struct sockaddr *)&receiver, sizeof(receiver));

                next_packet++;
                total_transmitted++;

                calculate_cycles(first_packet, next_packet, window_size, &c1, &c2);
                T = c1 + c2;

                print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);
                current_time += transmission_time;
            } else {
                print_state(current_time, acknowledgements_received, c1, c2, T, first_packet, next_packet, sequence_number_range);
            }
        }
    }

    printf("\nTransmission Completed.\n");
    printf("Total Transmitted   : %d\n", total_transmitted);
    printf("Total ACK            : %d\n", acknowledgements_received);
    printf("Current Window      : ");
    print_window(first_packet, next_packet, sequence_number_range);
    printf("\n");
    printf("Current Window Size : %d\n", T);

    free(send_times);
    close(socket_number);
    return 0;
}
