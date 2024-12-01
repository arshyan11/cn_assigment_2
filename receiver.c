#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 8080

void receive_file(int client_sock) {
    FILE *file;
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_received;
    char *file_name = "received_video.mp4";

    // Open the file to save the received video
    file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Receive the file in segments
    while ((bytes_received = recv(client_sock, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    // Close the file and socket
    fclose(file);
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // Listen for incoming connections
    listen(server_sock, 3);

    printf("Waiting for incoming connections...\n");

    // Accept an incoming connection
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("Accept failed");
        return -1;
    }

    printf("Connection accepted\n");

    // Receive the file
    receive_file(client_sock);

    // Clean up
    close(server_sock);

    return 0;
}
