// fake_scanner_udp.c
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[n-1] = '\0';
        n--;
    }
}

int main(void) {
    const int PORT = 12345;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    // Bind specifically to localhost (matches your Node server's HOST=127.0.0.1)
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed\n");
        return 1;
    }

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    printf("Fake UDP scanner listening on 127.0.0.1:%d\n", PORT);

    for (;;) 
    {
        char buf[1024];
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0,(struct sockaddr*)&client, &client_len);
        if (n < 0) { perror("recvfrom"); continue; }

        buf[n] = '\0';
        trim(buf);

        printf("RX: \"%s\"\n", buf);

        char reply[1024];
        snprintf(reply, sizeof(reply), "OK");

        // Your UI expects mode-reply to be "0", "1", or "2"
        if (strncmp(buf, "mode ", 5) == 0) {
            // Send back just the number portion
            snprintf(reply, sizeof(reply), "%s", buf + 5);
        } else if (strncmp(buf, "custom ", 7) == 0) {
            snprintf(reply, sizeof(reply), "CUSTOM_OK");
        } else if (strcmp(buf, "start") == 0) {
            snprintf(reply, sizeof(reply), "START_OK");
        } else if (strncmp(buf, "pause", 5) == 0) {
            snprintf(reply, sizeof(reply), "PAUSE_OK");
        } else if (strcmp(buf, "stop") == 0) {
            snprintf(reply, sizeof(reply), "STOP_OK");
        } else if (strcmp(buf, "shutdown") == 0) {
            snprintf(reply, sizeof(reply), "SHUTDOWN_OK");
        }

        sendto(sock, reply, strlen(reply), 0,
               (struct sockaddr*)&client, client_len);

        printf("TX: \"%s\"\n", reply);
    }

    close(sock);
    return 0;
}

//gcc -O2 -Wall -Wextra -o fake_scanner_udp fake_scanner_udp.c
//fake_scanner_udp