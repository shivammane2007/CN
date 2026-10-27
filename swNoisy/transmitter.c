#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

int main() {
    int sock;
    struct sockaddr_in receiver, sender;

    int size = 0, tp, tt, loss = 0;
    int buffer = 1, ack = 0;

    printf("Enter number of packets to transmit: ");
    if (scanf("%d", &size) != 1 || size <= 0) {
        printf("Invalid number of packets.\n");
        return 1;
    }

    printf("Enter Transmission Time (sec): ");
    if (scanf("%d", &tt) != 1 || tt < 0) {
        printf("Invalid transmission time.\n");
        return 1;
    }

    printf("Enter Propagation Time (sec): ");
    if (scanf("%d", &tp) != 1 || tp < 0) {
        printf("Invalid propagation time.\n");
        return 1;
    }

    printf("Enter Loss value (0 = no loss, N = every Nth packet lost after N-1 successes): ");
    if (scanf("%d", &loss) != 1 || loss < 0) {
        printf("Invalid loss value.\n");
        return 1;
    }

    // Configure receiver address
    memset(&receiver, 0, sizeof(receiver));
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Calculate RTT and timeout
    int rtt = 2 * tt + 2 * tp;
    int timeout_sec = 3 * rtt;

    // Ensure the timeout is at least one second
    if (timeout_sec < 1) {
        timeout_sec = 1;
    }

    // Enable socket receive timeout
    struct timeval sock_timeout;
    sock_timeout.tv_sec = timeout_sec;
    sock_timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   &sock_timeout, sizeof(sock_timeout)) < 0) {
        perror("Failed to set socket timeout");
        close(sock);
        return 1;
    }

    // Send connection parameters to receiver
    if (sendto(sock, &tt, sizeof(tt), 0,
               (struct sockaddr *)&receiver, sizeof(receiver)) < 0 ||
        sendto(sock, &tp, sizeof(tp), 0,
               (struct sockaddr *)&receiver, sizeof(receiver)) < 0 ||
        sendto(sock, &loss, sizeof(loss), 0,
               (struct sockaddr *)&receiver, sizeof(receiver)) < 0) {
        perror("Failed to send configuration");
        close(sock);
        return 1;
    }

    socklen_t len = sizeof(sender);
    time_t start_time = time(NULL);

    // Stop-and-Wait ARQ transmission
    while (buffer <= size) {
        // Simulate packet transmission time
        sleep(tt);

        printf("t%d: Packet %d sent to receiver\n",
               (int)difftime(time(NULL), start_time), buffer);

        // Send the current packet
        if (sendto(sock, &buffer, sizeof(buffer), 0,
                   (struct sockaddr *)&receiver,
                   sizeof(receiver)) < 0) {
            perror("Failed to send packet");
            close(sock);
            return 1;
        }

        // Wait for acknowledgement
        len = sizeof(sender);
        int n = recvfrom(sock, &ack, sizeof(ack), 0,
                         (struct sockaddr *)&sender, &len);

        if (n == sizeof(ack) && ack == 1) {
            // ACK received successfully
            sleep(tp);

            printf("t%d: Acknowledgement for Packet %d received from receiver\n",
                   (int)difftime(time(NULL), start_time), buffer);

            // Move to the next packet only after receiving its ACK
            buffer++;
        }
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // No ACK received within the timeout period
            printf("t%d: Timeout (waited %d sec = 3*RTT) - "
                   "Acknowledgement not received for Packet %d\n",
                   (int)difftime(time(NULL), start_time),
                   timeout_sec, buffer);

            printf("t%d: Retransmitting Packet %d\n",
                   (int)difftime(time(NULL), start_time), buffer);

            // buffer remains unchanged, so the same packet is retransmitted
        }
        else if (n < 0) {
            perror("Error receiving ACK");
            close(sock);
            return 1;
        }
        else {
            // An unexpected ACK was received; retry the current packet
            printf("t%d: Invalid ACK received for Packet %d. "
                   "Retransmitting...\n",
                   (int)difftime(time(NULL), start_time), buffer);
        }
    }

    printf("t%d: Transmission completed\n",
           (int)difftime(time(NULL), start_time));

    printf("Total time required: %d seconds\n",
           (int)difftime(time(NULL), start_time));

    close(sock);
    return 0;
}