#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>

#define MAX_N 26

int N = 0;
int cost[MAX_N][MAX_N];

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

bool recv_n(int fd, void* buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, (char*)buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}

void test_parse() {
    parse("oracle-config.txt");
    assert(N == 5);
    assert(cost[0][1] == 3);
    assert(cost[0][2] == 1);
    assert(cost[1][2] == 2);
    assert(cost[1][3] == 4);
    assert(cost[2][4] == 1);
    assert(cost[3][4] == 7);
    assert(cost[0][3] == -1);
    assert(cost[0][4] == -1);
    assert(cost[1][0] == 3);
    assert(cost[4][2] == 1);
    printf("test_parse: OK\n");
}

void test_recv_n() {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    const char* msg = "hello";
    send(sv[0], msg, 5, 0);

    char buf[5];
    assert(recv_n(sv[1], buf, 5));
    assert(memcmp(buf, "hello", 5) == 0);

    close(sv[0]);
    close(sv[1]);
    printf("test_recv_n: OK\n");
}

int main() {
    test_parse();
    test_recv_n();
    printf("all tests passed\n");
    return 0;
}