#ifndef BROKER_H
#define BROKER_H

#include <pthread.h>

#define MAX_TOPIC_LEN 128
#define MAX_DATA_LEN 512
#define MAX_GATEWAY_ID 32

// ====================== STRUCTS ======================

typedef struct TopicMessage {
    char topic[MAX_TOPIC_LEN];
    char data[MAX_DATA_LEN];
    long timestamp;
    struct TopicMessage* next;
} TopicMessage;

typedef struct GatewayClient {
    int socket;
    char id[MAX_GATEWAY_ID];
    pthread_t thread;
    struct GatewayClient* next;
} GatewayClient;

typedef struct Broker {
    int server_socket;
    int port;

    GatewayClient* gateways;
    TopicMessage* history;

    pthread_mutex_t mutex_gateways;
    pthread_mutex_t mutex_history;

    int running;
} Broker;

// ==================== API DEL BROKER ====================

int broker_init(Broker* broker, int port);
void broker_start(Broker* broker);
void broker_stop(Broker* broker);
void broker_cleanup(Broker* broker);

// Debug
void broker_print_history(Broker* broker);
void broker_print_gateways(Broker* broker);

#endif

