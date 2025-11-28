#include "gateway.h"

// ==================== FUNCIONES INTERNAS ====================

// Buscar publisher por socket
static PublisherInfo* _find_publisher(Gateway* gw, int socket) {
    pthread_mutex_lock(&gw->publishers_mutex);

    PublisherInfo* p = gw->publishers;
    while (p != NULL) {
        if (p->socket == socket) {
            pthread_mutex_unlock(&gw->publishers_mutex);
            return p;
        }
        p = p->next;
    }

    pthread_mutex_unlock(&gw->publishers_mutex);
    return NULL;
}

// Procesar datos de sensor
static void _process_sensor_data(Gateway* gw, PublisherInfo* p, const char* raw) {
    printf("[GATEWAY] ?? [Publisher %s] %s\n", p->publisher_id, raw);

    char sensor[50];
    float value;

    if (sscanf(raw, "%[^:]:%f", sensor, &value) != 2) {
        printf("[GATEWAY] ? Formato inválido: %s\n", raw);
        return;
    }

    SensorData data;
    strncpy(data.publisher_id, p->publisher_id, sizeof(data.publisher_id));
    strncpy(data.sensor_type, sensor, sizeof(data.sensor_type));
    data.value = value;
    data.timestamp = time(NULL);

    message_queue_enqueue(gw->queue, data);

    gw->total_messages_received++;

    printf("[GATEWAY] ? Procesado: %s = %.2f\n", sensor, value);
}

// Registrar publisher
static void _publisher_register(PublisherInfo* p, const char* msg) {
    if (strncmp(msg, "REGISTER ", 9) != 0) return;

    const char* id = msg + 9;
    strncpy(p->publisher_id, id, sizeof(p->publisher_id));

    printf("[GATEWAY] ? Publisher registrado como %s\n", id);

    char ack[200];
    snprintf(ack, sizeof(ack), "REGACK %s OK\n", id);
    send(p->socket, ack, strlen(ack), 0);
}

// Hilo por publisher
static void* _publisher_thread(void* arg) {
    PublisherInfo* p = (PublisherInfo*)arg;
    Gateway* gw = p->gateway;

    char buf[1024];

    send(p->socket, "Bienvenido. Use REGISTER <id>\n", 31, 0);

    while (p->connected) {
        memset(buf, 0, sizeof(buf));

        int n = recv(p->socket, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("[GATEWAY] Publisher %s desconectado\n", p->publisher_id);
            break;
        }

        char* line = strtok(buf, "\n");
        while (line) {
            if (strlen(p->publisher_id) == 0)
                _publisher_register(p, line);
            else
                _process_sensor_data(gw, p, line);

            line = strtok(NULL, "\n");
        }
    }

    close(p->socket);
    p->connected = 0;
    return NULL;
}

// Hilo principal de procesamiento
static void* _queue_processor(void* arg) {
    Gateway* gw = (Gateway*)arg;

    while (gw->running) {
        pthread_mutex_lock(&gw->queue->mutex);

        while (message_queue_is_empty(gw->queue) && gw->running)
            pthread_cond_wait(&gw->queue->not_empty, &gw->queue->mutex);

        if (!gw->running) {
            pthread_mutex_unlock(&gw->queue->mutex);
            break;
        }

        SensorData d = message_queue_dequeue(gw->queue);
        pthread_mutex_unlock(&gw->queue->mutex);

        char topic[200];
        snprintf(topic, sizeof(topic),
                 "gateway/%s/publisher/%s/sensor/%s",
                 gw->gateway_id, d.publisher_id, d.sensor_type);

        char json[400];
        snprintf(json, sizeof(json),
                 "{\"value\":%.2f,\"timestamp\":%ld}",
                 d.value, d.timestamp);

        if (gateway_send_to_broker(gw, topic, json) == 0)
            gw->total_messages_sent++;
    }

    return NULL;
}

// ==================== COLA ====================

MessageQueue* message_queue_create(void) {
    MessageQueue* q = malloc(sizeof(MessageQueue));
    q->front = q->rear = NULL;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

void message_queue_enqueue(MessageQueue* q, SensorData d) {
    QueueNode* n = malloc(sizeof(QueueNode));
    n->data = d;
    n->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (!q->rear) q->front = q->rear = n;
    else {
        q->rear->next = n;
        q->rear = n;
    }
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

SensorData message_queue_dequeue(MessageQueue* q) {
    QueueNode* n = q->front;
    SensorData d = n->data;

    q->front = n->next;
    if (!q->front) q->rear = NULL;

    free(n);
    q->count--;
    return d;
}

int message_queue_is_empty(MessageQueue* q) {
    return q->front == NULL;
}

// ==================== API DEL GATEWAY ====================

int gateway_init(Gateway* gw, const char* id, int port) {
    memset(gw, 0, sizeof(Gateway));
    strcpy(gw->gateway_id, id);

    gw->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(gw->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(gw->server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    listen(gw->server_socket, MAX_PUBLISHERS);

    pthread_mutex_init(&gw->publishers_mutex, NULL);
    gw->queue = message_queue_create();
    gw->running = 1;

    return 0;
}

int gateway_connect_to_broker(Gateway* gw, const char* ip, int port) {
    gw->broker_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(gw->broker_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    char msg[100];
    snprintf(msg, sizeof(msg), "REGISTER GATEWAY %s\n", gw->gateway_id);
    send(gw->broker_socket, msg, strlen(msg), 0);

    return 0;
}

int gateway_send_to_broker(Gateway* gw, const char* topic, const char* message) {
    char out[600];
    snprintf(out, sizeof(out), "PUBLISH %s %s\n", topic, message);

    return send(gw->broker_socket, out, strlen(out), 0) < 0 ? -1 : 0;
}

void gateway_add_publisher(Gateway* gw, int sock, struct sockaddr_in addr) {
    PublisherInfo* p = malloc(sizeof(PublisherInfo));
    memset(p, 0, sizeof(PublisherInfo));

    p->socket = sock;
    p->address = addr;
    p->connected = 1;
    p->gateway = gw;

    pthread_mutex_lock(&gw->publishers_mutex);
    p->next = gw->publishers;
    gw->publishers = p;
    pthread_mutex_unlock(&gw->publishers_mutex);

    pthread_create(&p->thread_id, NULL, _publisher_thread, p);
}

void gateway_start(Gateway* gw) {
    pthread_t processor;
    pthread_create(&processor, NULL, _queue_processor, gw);

    while (gw->running) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int cs = accept(gw->server_socket, (struct sockaddr*)&caddr, &clen);
        if (cs >= 0)
            gateway_add_publisher(gw, cs, caddr);
    }

    pthread_join(processor, NULL);
}

void gateway_stop(Gateway* gw) {
    gw->running = 0;
    close(gw->server_socket);

    pthread_mutex_lock(&gw->queue->mutex);
    pthread_cond_signal(&gw->queue->not_empty);
    pthread_mutex_unlock(&gw->queue->mutex);
}

void gateway_cleanup(Gateway* gw) {
    PublisherInfo* p = gw->publishers;
    while (p) {
        PublisherInfo* nx = p->next;
        close(p->socket);
        free(p);
        p = nx;
    }

    message_queue_cleanup(gw->queue);
    pthread_mutex_destroy(&gw->publishers_mutex);
}

void gateway_print_stats(Gateway* gw) {
    printf("\n=== STATS %s ===\n", gw->gateway_id);
    printf("Mensajes recibidos: %d\n", gw->total_messages_received);
    printf("Mensajes enviados: %d\n", gw->total_messages_sent);
}

