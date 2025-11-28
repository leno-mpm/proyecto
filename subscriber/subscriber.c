#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int server_socket;

// ==============================
// Hilo escuchando mensajes
// ==============================
void* listener_thread(void* arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int r = recv(server_socket, buffer, sizeof(buffer), 0);
        
        if (r <= 0) {
            printf("[SUBSCRIBER] Desconectado del broker.\n");
            close(server_socket);
            exit(0);
        }

        printf("[MESSAGE RECEIVED] %s\n", buffer);
    }

    return NULL;
}

// ==============================
// Programa principal
// ==============================
int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Uso: %s <ip_broker> <puerto> <topic>\n", argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];

    // Crear socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Conectar
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("No se pudo conectar al broker");
        return 1;
    }

    printf("[SUBSCRIBER] Conectado al broker %s:%d\n", ip, port);

    // Enviar comando SUBSCRIBE
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "SUBSCRIBE %s\n", topic);
    send(server_socket, cmd, strlen(cmd), 0);

    printf("[SUBSCRIBER] Suscrito al topic: %s\n", topic);

    // Lanzar hilo listener
    pthread_t th;
    pthread_create(&th, NULL, listener_thread, NULL);
    pthread_detach(th);

    // Mantener vivo
    while (1) {
        sleep(1);
    }

    return 0;
}
