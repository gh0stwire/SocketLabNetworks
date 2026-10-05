#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    printf("Connecting to 127.0.0.1:5000...\n");
    fflush(stdout);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }

    printf("Connected. Waiting for data...\n");
    fflush(stdout);

    char buf[100];
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n < 0) { perror("recv"); exit(1); }

    printf("Got %d bytes: ", n);
    fwrite(buf, 1, n, stdout);
    printf("\n");

    close(fd);
    return 0;
}
