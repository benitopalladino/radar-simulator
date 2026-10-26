#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT          5050
#define NUM_TARGETS   10
#define NUM_NOISE     30
#define MAX_CONTACTS  (NUM_TARGETS + NUM_NOISE)
#define THRESHOLD     8.0
#define NOISE_MAX     6.0
#define K_RADAR       1e8

typedef struct {
    int id;
    double range_km;
    double azimuth_deg;
    double amplitude;
} Contact;

static double rand_between(double min, double max) {
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

static void generate_scan(Contact *c) {
    for (int i = 0; i < MAX_CONTACTS; i++) {
        double r = rand_between(1.0, 100.0);
        double amp = rand_between(0.0, NOISE_MAX);
        if (i < NUM_TARGETS) {
            amp += K_RADAR / (r * r * r * r);
        }
        c[i] = (Contact){ i + 1, r, rand_between(0.0, 359.0), amp };
    }
}

static int send_scan(int fd, int scan_no) {
    Contact c[MAX_CONTACTS];
    char line[128];
    int n;

    generate_scan(c);
    n = snprintf(line, sizeof line, "SCAN %d\n", scan_no);
    if (write(fd, line, n) < 0) return -1;

    for (int i = 0; i < MAX_CONTACTS; i++) {
        if (c[i].amplitude > THRESHOLD) {
            n = snprintf(line, sizeof line, "%d,%.1f,%.1f,%.1f\n",
                         c[i].id, c[i].range_km, c[i].azimuth_deg, c[i].amplitude);
            if (write(fd, line, n) < 0) return -1;
        }
    }
    return (write(fd, "END\n", 4) < 0) ? -1 : 0;
}

int main(void) {
    int server_fd, client_fd, opt = 1;
    struct sockaddr_in addr;

    srand((unsigned)time(NULL));
    signal(SIGPIPE, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, 1) < 0) { perror("listen"); return 1; }
    printf("Server radar in ascolto sulla porta %d\n", PORT);

    for (;;) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) { perror("accept"); continue; }
        printf("Client connesso\n");
        for (int scan = 1; send_scan(client_fd, scan) == 0; scan++) {
            sleep(1);
        }
        close(client_fd);
        printf("Client disconnesso\n");
    }
}