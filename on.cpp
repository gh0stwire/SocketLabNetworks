#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#define MAX_N 26

int N = 0;
int cost[MAX_N][MAX_N];
int conn_fds[MAX_N];
uint32_t vn_ip[MAX_N];
uint16_t vn_port[MAX_N];

void parse(const char* path) {
    for (int i = 0; i < MAX_N; i++)
        for (int j = 0; j < MAX_N; j++)
            cost[i][j] = -1;

    FILE* f = fopen(path, "r");
    if (!f) { perror("fopen"); exit(1); }

    char line[1024];
    int row = 0;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        int col = row + 1;
        for (char* tok = strtok(p, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
            int c = atoi(tok);
            if (c != -1) cost[row][col] = cost[col][row] = c;
            col++;
        }
        if (!N) N = col;
        row++;
    }
    fclose(f);
}

void print_matrix() {
    printf("N=%d\n", N);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (cost[i][j] == -1) printf("-\t");
            else                  printf("%d\t", cost[i][j]);
        }
        printf("\n");
    }
    fflush(stdout);
}

bool recv_n(int fd, void* buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, (char*)buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}

bool read_connect(int idx) {
    unsigned char buf[7];
    if (!recv_n(conn_fds[idx], buf, 7)) return false;
    if (buf[0] != 1) return false;

    uint32_t ipn;
    uint16_t pn;
    memcpy(&ipn, buf + 1, 4);
    memcpy(&pn,  buf + 5, 2);
    vn_ip[idx]   = ntohl(ipn);
    vn_port[idx] = ntohs(pn);

    char ipstr[INET_ADDRSTRLEN];
    struct in_addr ia{};
    ia.s_addr = ipn;
    inet_ntop(AF_INET, &ia, ipstr, sizeof(ipstr));
    printf("%c connected: %s:%u\n", 'A' + idx, ipstr, vn_port[idx]);
    fflush(stdout);
    return true;
}

void send_ls(int idx) {
    int me = idx, n = 1;
    for (int j = 0; j < N; j++)
        if (j != me && cost[me][j] != -1) n++;

    unsigned char buf[2 + MAX_N * 11];
    unsigned char* p = buf;
    *p++ = 2;
    *p++ = n;

    auto put = [&](int node, uint32_t ip, uint16_t port, uint32_t c) {
        *p++ = (unsigned char)('A' + node);
        uint32_t ipn = htonl(ip);     memcpy(p, &ipn, 4); p += 4;
        uint16_t pn  = htons(port);   memcpy(p, &pn,  2); p += 2;
        uint32_t cn  = htonl(c);      memcpy(p, &cn,  4); p += 4;
    };

    put(me, vn_ip[me], vn_port[me], 0);
    for (int j = 0; j < N; j++)
        if (j != me && cost[me][j] != -1)
            put(j, vn_ip[j], vn_port[j], cost[me][j]);

    send(conn_fds[idx], buf, p - buf, 0);
    printf("%c: sent %ld bytes\n", 'A' + idx, p - buf);
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <config>\n", argv[0]);
        return 1;
    }
    parse(argv[1]);
    print_matrix();

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(5001);
    a.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(lfd, (sockaddr*)&a, sizeof(a)) < 0) { perror("bind"); return 1; }
    if (listen(lfd, 5) < 0) { perror("listen"); return 1; }

    printf("listening on 5001, waiting for %d VNs\n", N);
    fflush(stdout);

    for (int i = 0; i < N; i++) {
        conn_fds[i] = accept(lfd, NULL, NULL);
        if (conn_fds[i] < 0) { perror("accept"); return 1; }
        if (!read_connect(i)) {
            fprintf(stderr, "%c: bad CONNECT\n", 'A' + i);
            return 1;
        }
    }

    for (int i = 0; i < N; i++) send_ls(i);

    printf("done. sleeping.\n");
    fflush(stdout);
    while (1) sleep(1);
}