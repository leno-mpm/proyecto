#include "broker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// =========================================================
// MANEJO DE LISTAS
// =========================================================

static void add_gateway(Broker* broker, int socket, const char* id) {
    GatewayClient* g = malloc(sizeof(GatewayClient));
    g->socket = socket;
    strncpy(g->id, id, MAX_GATEWAY_ID);
    g->next = NULL;

    pthread_mutex_lock(&broker->mutex_gateways);

    g->next = broker->gateways;
    broker->gateways = g;

    pthread_mutex_unlock(&broker->mutex_gateways);

    printf("[BROKER] Gateway registrado: %s\n", id);
}

static void save_message(Broker* broker, const char* topic, const char* data) {
    TopicMessage* m = malloc(sizeof(TopicMessage));
    strncpy(m->topic, topic, MAX_TOPIC_LEN);
    strncpy(m->data, data, MAX_DATA_LEN);
    m->timestamp = time(NULL);
    m->next = NULL;

    pthread_mutex_lock(&broker->mutex_history);

    m->next = broker->history;
    broker->history = m;

    pthread_mutex_unlock(&broker->mutex_history);
}


// =========================================================
// THREAD DEL CLIENTE
// =========================================================

typedef struct {
    Broker* broker;
    int client_socket;
} ClientArgs;

static void* client_thread(void* arg) {
    ClientArgs* args = (ClientArgs*)arg;
    Broker* broker = args->broker;
    int client_socket = args->client_socket;
    free(arg);

    char buffer[1024];

    printf("[BROKER] Nuevo thread manejando cliente (socket %d)\n", client_socket);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int r = recv(client_socket, buffer, sizeof(buffer), 0);

        if (r <= 0) {
            printf("[BROKER] Cliente desconectado (socket %d)\n", client_socket);
            close(client_socket);
            return NULL;
        }

        if (strncmp(buffer, "REGISTER GATEWAY", 16) == 0) {
            char id[64];
            sscanf(buffer, "REGISTER GATEWAY %s", id);
            add_gateway(broker, client_socket, id);
            send(client_socket, "OK REGISTERED\n", 14, 0);
        }

        else if (strncmp(buffer, "PUBLISH", 7) == 0) {
            char topic[128], data[512];
            sscanf(buffer, "PUBLISH %s %512[^\n]", topic, data);

            printf("[BROKER] PUBLISH recibido:\n");
            printf("         Topic: %s\n", topic);
            printf("         Data:  %s\n\n", data);

            save_message(broker, topic, data);
        }

        else {
            printf("[BROKER] Comando desconocido: %s\n", buffer);
        }
    }
}


// =========================================================
// SERVIDOR PRINCIPAL
// =========================================================

int broker_init(Broker* broker, int port) {
    broker->port = port;
    broker->gateways = NULL;
    broker->history = NULL;
    broker->running = 1;

    pthread_mutex_init(&broker->mutex_gateways, NULL);
    pthread_mutex_init(&broker->mutex_history, NULL);

    return 1;
}

void broker_start(Broker* broker) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    broker->server_socket = server_fd;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(broker->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("[BROKER] Servidor iniciado en puerto %d\n", broker->port);

    while (broker->running) {
        struct sockaddr_in client;
        socklen_t c = sizeof(client);

        int client_socket = accept(server_fd, (struct sockaddr*)&client, &c);
        printf("[BROKER] Nueva conexión aceptada (socket %d)\n", client_socket);

        ClientArgs* args = malloc(sizeof(ClientArgs));
        args->broker = broker;
        args->client_socket = client_socket;

        pthread_t th;
        pthread_create(&th, NULL, client_thread, args);
        pthread_detach(th);
    }
}

void broker_stop(Broker* broker) {
    broker->running = 0;
}

void broker_cleanup(Broker* broker) {
    close(broker->server_socket);
    pthread_mutex_destroy(&broker->mutex_gateways);
    pthread_mutex_destroy(&broker->mutex_history);
}


// =========================================================
// DEBUG
// =========================================================

void broker_print_history(Broker* broker) {
    pthread_mutex_lock(&broker->mutex_history);

    TopicMessage* m = broker->history;
    printf("=== HISTORIAL ===\n");
    while (m) {
        printf("[%ld] %s -> %s\n", m->timestamp, m->topic, m->data);
        m = m->next;
    }

    pthread_mutex_unlock(&broker->mutex_history);
}

void broker_print_gateways(Broker* broker) {
    pthread_mutex_lock(&broker->mutex_gateways);

    GatewayClient* g = broker->gateways;
    printf("=== GATEWAYS REGISTRADOS ===\n");
    while (g) {
        printf(" - %s\n", g->id);
        g = g->next;
    }

    pthread_mutex_unlock(&broker->mutex_gateways);
}

