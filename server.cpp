#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(listen_fd, 5) < 0) { perror("listen"); exit(1); }

    printf("Listening on 127.0.0.1:5000...\n");
    fflush(stdout);   

    int conn_fd = accept(listen_fd, nullptr, nullptr);
    if (conn_fd < 0) { perror("accept"); exit(1); }

    printf("Client connected.\n");
    fflush(stdout);

    send(conn_fd, "hi", 2, 0);
    printf("Sent 'hi'. Closing.\n");
    fflush(stdout);

    close(conn_fd);
    close(listen_fd);
    return 0;
}
