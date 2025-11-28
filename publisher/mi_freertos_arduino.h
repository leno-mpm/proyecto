#ifndef MI_FREERTOS_ARDUINO_H
#define MI_FREERTOS_ARDUINO_H

#include <Arduino.h>

// ==================== COMPATIBILIDAD CON FreeRTOS ====================

// Si estamos en Wokwi/Arduino, usar FreeRTOS real de ESP32
// Pero con la API de tu implementación

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Mapear tus funciones a FreeRTOS real de ESP32
#define xTaskCreate mi_xTaskCreate
#define vTaskDelay mi_vTaskDelay
#define xTaskGetTickCount mi_xTaskGetTickCount
#define xQueueCreate mi_xQueueCreate
#define xQueueSend mi_xQueueSend
#define xQueueReceive mi_xQueueReceive
#define uxQueueMessagesWaiting mi_uxQueueMessagesWaiting
#define xSemaphoreCreateMutex mi_xSemaphoreCreateMutex
#define xSemaphoreTake mi_xSemaphoreTake
#define xSemaphoreGive mi_xSemaphoreGive
#define taskYIELD mi_taskYIELD

// Wrappers para compatibilidad
BaseType_t mi_xTaskCreate(TaskFunction_t pxTaskCode,
                         const char *pcName,
                         const uint16_t usStackDepth,
                         void *pvParameters,
                         UBaseType_t uxPriority,
                         TaskHandle_t *pxCreatedTask) {
    return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
}

void mi_vTaskDelay(const TickType_t xTicksToDelay) {
    vTaskDelay(xTicksToDelay);
}

TickType_t mi_xTaskGetTickCount(void) {
    return xTaskGetTickCount();
}

QueueHandle_t mi_xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize) {
    return xQueueCreate(uxQueueLength, uxItemSize);
}

BaseType_t mi_xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    return xQueueSend(xQueue, pvItemToQueue, xTicksToWait);
}

BaseType_t mi_xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    return xQueueReceive(xQueue, pvBuffer, xTicksToWait);
}

UBaseType_t mi_uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    return uxQueueMessagesWaiting(xQueue);
}

SemaphoreHandle_t mi_xSemaphoreCreateMutex(void) {
    return xSemaphoreCreateMutex();
}

BaseType_t mi_xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
    return xSemaphoreTake(xSemaphore, xBlockTime);
}

BaseType_t mi_xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    return xSemaphoreGive(xSemaphore);
}

void mi_taskYIELD(void) {
    taskYIELD();
}

#else
// Si no es ESP32, usar tu implementación de FreeRTOS
#include "mi_freertos.h"
#endif

#endif
