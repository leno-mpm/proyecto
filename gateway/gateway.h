#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_PUBLISHERS 100

// ====================== ESTRUCTURAS ==========================

// Datos producidos por un publisher
typedef struct {
    char publisher_id[50];
    char sensor_type[50];
    float value;
    long timestamp;
} SensorData;

// Nodo de la cola
typedef struct QueueNode {
    SensorData data;
    struct QueueNode* next;
} QueueNode;

// Cola de mensajes
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} MessageQueue;

// Información de cada publisher conectado
typedef struct PublisherInfo {
    int socket;
    struct sockaddr_in address;
    char publisher_id[50];
    int connected;
    pthread_t thread_id;
    struct Gateway* gateway; // <--- NECESARIO
    struct PublisherInfo* next;
} PublisherInfo;

// Datos principales del Gateway
typedef struct Gateway {
    char gateway_id[50];

    int server_socket;
    int broker_socket;

    MessageQueue* queue;

    PublisherInfo* publishers;

    pthread_mutex_t publishers_mutex;

    int connected_publishers;
    int total_messages_received;
    int total_messages_sent;

    int running;
} Gateway;

// ====================== APIs PÚBLICAS ==========================

int gateway_init(Gateway* gateway, const char* id, int port);
int gateway_connect_to_broker(Gateway* gateway, const char* ip, int port);

void gateway_start(Gateway* gateway);
void gateway_stop(Gateway* gateway);
void gateway_cleanup(Gateway* gateway);

void gateway_add_publisher(Gateway* gateway, int socket, struct sockaddr_in addr);
void gateway_remove_publisher(Gateway* gateway, int socket);

int gateway_send_to_broker(Gateway* gateway, const char* topic, const char* message);

void gateway_print_stats(Gateway* gateway);

// Cola
MessageQueue* message_queue_create(void);
void message_queue_enqueue(MessageQueue* queue, SensorData data);
SensorData message_queue_dequeue(MessageQueue* queue);
int message_queue_is_empty(MessageQueue* queue);
void message_queue_cleanup(MessageQueue* queue);

#endif

