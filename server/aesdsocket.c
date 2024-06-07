#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#define PORT 9000
#define BACKLOG 5
#define BUFFER_SIZE 1024

int server_fd;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        close(server_fd);
        remove("/var/tmp/aesdsocketdata");
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    FILE *file;

    // Setup signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Open syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Failed to bind socket");
        close(server_fd);
        return -1;
    }

    // Listen for connections
    if (listen(server_fd, BACKLOG) == -1) {
        syslog(LOG_ERR, "Failed to listen on socket");
        close(server_fd);
        return -1;
    }

    syslog(LOG_INFO, "Server started on port %d", PORT);

    // Accept connections
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd == -1) {
            syslog(LOG_ERR, "Failed to accept connection");
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        // Open file for appending
        file = fopen("/var/tmp/aesdsocketdata", "a+");
        if (file == NULL) {
            syslog(LOG_ERR, "Failed to open file");
            close(client_fd);
            continue;
        }

        // Receive data and write to file
        while (1) {
            ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received == -1) {
                syslog(LOG_ERR, "Failed to receive data");
                break;
            } else if (bytes_received == 0) {
                break;
            }

            buffer[bytes_received] = '\0';
            fprintf(file, "%s", buffer);
            fflush(file);

            // Send the file content back to client
            fseek(file, 0, SEEK_SET);
            while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
                send(client_fd, buffer, strlen(buffer), 0);
            }
        }

        // Close file and client connection
        fclose(file);
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    }

    // Close syslog
    closelog();
    close(server_fd);
    return 0;
}
