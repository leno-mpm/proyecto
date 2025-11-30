#ifndef BROKER_H
#define BROKER_H

#include <pthread.h>
#include <time.h>

#define MAX_GATEWAY_ID 64
#define MAX_TOPIC_LEN 128
#define MAX_DATA_LEN 512

// ----------------------
// Estructuras
// ----------------------

typedef struct GatewayClient {
    int socket;
    char id[MAX_GATEWAY_ID];
    struct GatewayClient* next;
} GatewayClient;

typedef struct SubscriberClient {
    int socket;
    char topic[MAX_TOPIC_LEN];
    struct SubscriberClient* next;
} SubscriberClient;

typedef struct TopicMessage {
    char topic[MAX_TOPIC_LEN];
    char data[MAX_DATA_LEN];
    time_t timestamp;
    struct TopicMessage* next;
} TopicMessage;

typedef struct {
    int port;
    int server_socket;
    int running;

    GatewayClient* gateways;
    SubscriberClient* subscribers;
    TopicMessage* history;

    pthread_mutex_t mutex_gateways;
    pthread_mutex_t mutex_subscribers;
    pthread_mutex_t mutex_history;

} Broker;

// ----------------------
// Funciones del Broker
// ----------------------

int  broker_init(Broker* broker, int port);
void broker_start(Broker* broker);
void broker_stop(Broker* broker);
void broker_cleanup(Broker* broker);

// Debug
void broker_print_history(Broker* broker);
void broker_print_gateways(Broker* broker);

#endif

