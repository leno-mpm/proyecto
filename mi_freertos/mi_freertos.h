#ifndef MI_FREERTOS_H
#define MI_FREERTOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

// ==================== DEFINICIONES BÁSICAS ====================
#define pdTRUE      1
#define pdFALSE     0
#define pdPASS      1
#define pdFAIL      0

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef unsigned long TickType_t;  // usar %lu

#define portTICK_PERIOD_MS     1
#define portMAX_DELAY          (0xFFFFFFFFUL)

// Tipos de datos FreeRTOS
typedef void * TaskHandle_t;
typedef void * QueueHandle_t;
typedef void * SemaphoreHandle_t;

// Estados de tarea
typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} eTaskState;

// Estructura del Control Block de Tarea (TCB)
typedef struct tmiTaskControlBlock {
    // Identificación
    pthread_t xThreadId;
    char pcTaskName[32];

    // Funciones y parámetros
    void (*pvTaskCode)(void *);
    void *pvParameters;

    // Prioridad y estado
    UBaseType_t uxPriority;
    eTaskState eCurrentState;
    UBaseType_t uxBasePriority;

    // Tiempos y delays
    TickType_t xWakeTime;

    // Lista enlazada
    struct tmiTaskControlBlock *pxNext;

    // Stack (para simulación)
    void *pvStack;
    uint16_t usStackDepth;

    // Synchronization for suspend/resume
    pthread_mutex_t suspend_mutex;
    pthread_cond_t suspend_cond;
    int suspended; // 0 = running, 1 = suspended

    // alive flag
    int alive;

} miTCB;

// ==================== QUEUE (FIFO) ====================
typedef struct {
    void *buffer;             // pointer to contiguous memory
    UBaseType_t item_size;    // size of each item
    UBaseType_t length;       // max items
    UBaseType_t head;         // read index
    UBaseType_t tail;         // write index
    UBaseType_t count;        // items in queue
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} miQueue;

// ==================== DECLARACIÓN DE FUNCIONES ====================

// Gestión de Tareas
BaseType_t xTaskCreate(void (*pxTaskCode)(void *),
                      const char *pcName,
                      unsigned short usStackDepth,
                      void *pvParameters,
                      UBaseType_t uxPriority,
                      TaskHandle_t *pxCreatedTask);

void vTaskStartScheduler(void);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
void vTaskDelete(TaskHandle_t xTaskToDelete);

// Gestión del Tiempo
TickType_t xTaskGetTickCount(void);

// Colas (Queues)
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue);

// Semaforos (mutex)
SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

// Utilidades
void taskYIELD(void);

#endif /* MI_FREERTOS_H */

