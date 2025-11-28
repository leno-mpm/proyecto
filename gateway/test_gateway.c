#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "gateway.h"

// ==================== TEST MANUAL DEL GATEWAY ====================

void print_usage() {
    printf("Uso: ./test_gateway <gateway_id> [puerto]\n");
    printf("Ejemplo: ./test_gateway gw1 8080\n");
    printf("Puerto por defecto: 8080\n");
}

void* stats_thread(void* arg) {
    Gateway* gateway = (Gateway*)arg;
    
    while (gateway->running) {
        sleep(15); // Cada 15 segundos
        gateway_print_stats(gateway);
        gateway_list_publishers(gateway);
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    char gateway_id[50] = "gw1";
    int port = 8080;
    
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    strncpy(gateway_id, argv[1], sizeof(gateway_id) - 1);
    
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            printf("Error: Puerto inválido\n");
            return 1;
        }
    }
    
    printf("=========================================\n");
    printf("    TEST GATEWAY MQTT\n");
    printf("    ID: %s, Puerto: %d\n", gateway_id, port);
    printf("=========================================\n\n");
    
    Gateway gateway;
    
    // Inicializar gateway
    if (gateway_init(&gateway, gateway_id, port) != 0) {
        printf("Error inicializando gateway\n");
        return 1;
    }
    
    // Conectar al broker
    if (gateway_connect_to_broker(&gateway, BROKER_IP, BROKER_PORT) != 0) {
        printf("Error conectando al broker\n");
        gateway_cleanup(&gateway);
        return 1;
    }
    
    // Thread para mostrar estadísticas periódicamente
    pthread_t stats_tid;
    if (pthread_create(&stats_tid, NULL, stats_thread, &gateway) != 0) {
        printf("Error creando thread de estadísticas\n");
        gateway_cleanup(&gateway);
        return 1;
    }
    
    printf("[GATEWAY] Gateway %s ejecutándose. Presiona Ctrl+C para detener.\n", gateway_id);
    printf("[GATEWAY] Esperando conexiones de publishers en puerto %d...\n", port);
    printf("[GATEWAY] Los publishers deben usar el formato: \"sensor_type:value\"\n");
    printf("[GATEWAY] Ejemplo: \"temperature:25.5\" o \"humidity:60.0\"\n\n");
    
    // Iniciar gateway (bloqueante)
    gateway_start(&gateway);
    
    // Limpiar
    gateway_cleanup(&gateway);
    pthread_join(stats_tid, NULL);
    
    printf("[GATEWAY] Test completado\n");
    return 0;
}
