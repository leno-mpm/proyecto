#include "mi_freertos.h"
#include <sys/time.h>
#include <errno.h>

// ==================== VARIABLES GLOBALES ====================
static miTCB *pxReadyTasksList = NULL;
static volatile TickType_t xTickCount = 0;
static int schedulerRunning = 0;
static pthread_mutex_t global_tcb_mutex = PTHREAD_MUTEX_INITIALIZER;

// ==================== FUNCIONES INTERNAS ====================

static miTCB* _find_tcb_by_thread(pthread_t tid) {
    pthread_mutex_lock(&global_tcb_mutex);
    miTCB *it = pxReadyTasksList;
    while (it) {
        if (pthread_equal(it->xThreadId, tid)) {
            pthread_mutex_unlock(&global_tcb_mutex);
            return it;
        }
        it = it->pxNext;
    }
    pthread_mutex_unlock(&global_tcb_mutex);
    return NULL;
}

void* _miTaskWrapper(void *pvParameters) {
    miTCB *pxTask = (miTCB *)pvParameters;
    // set thread id
    pxTask->xThreadId = pthread_self();
    pxTask->eCurrentState = TASK_RUNNING;
    pxTask->alive = 1;

    printf("[FreeRTOS] >> INICIANDO: %s (thread %lu)\n", pxTask->pcTaskName, (unsigned long)pxTask->xThreadId);

    // Enter cooperative suspend check before running
    pthread_mutex_lock(&pxTask->suspend_mutex);
    while (pxTask->suspended) {
        // Wait until resumed
        pthread_cond_wait(&pxTask->suspend_cond, &pxTask->suspend_mutex);
    }
    pthread_mutex_unlock(&pxTask->suspend_mutex);

    // Allow cancellation at defined points
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // Call the actual task code
    pxTask->pvTaskCode(pxTask->pvParameters);

    // task finished
    pxTask->alive = 0;
    pxTask->eCurrentState = TASK_SUSPENDED;
    printf("[FreeRTOS] << COMPLETADA: %s\n", pxTask->pcTaskName);
    return NULL;
}

void _miAddTaskToReadyList(miTCB *pxTask) {
    pthread_mutex_lock(&global_tcb_mutex);
    pxTask->pxNext = pxReadyTasksList;
    pxReadyTasksList = pxTask;
    pthread_mutex_unlock(&global_tcb_mutex);
}

// ==================== GESTION DE TIEMPO ====================
TickType_t xTaskGetTickCount(void) {
    return xTickCount;
}

// Helper to add ms to timespec
static void _timespec_add_ms(struct timespec *t, long ms) {
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000L;
    if (t->tv_nsec >= 1000000000L) {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000L;
    }
}

// ==================== FUNCIONES PÚBLICAS ====================

BaseType_t xTaskCreate(void (*pxTaskCode)(void *),
                      const char *pcName,
                      unsigned short usStackDepth,
                      void *pvParameters,
                      UBaseType_t uxPriority,
                      TaskHandle_t *pxCreatedTask) {

    miTCB *pxNewTCB = (miTCB *)malloc(sizeof(miTCB));
    if (pxNewTCB == NULL) return pdFAIL;

    // Inicializar TCB
    memset(pxNewTCB, 0, sizeof(miTCB));
    strncpy(pxNewTCB->pcTaskName, pcName, sizeof(pxNewTCB->pcTaskName) - 1);
    pxNewTCB->pvTaskCode = pxTaskCode;
    pxNewTCB->pvParameters = pvParameters;
    pxNewTCB->uxPriority = uxPriority;
    pxNewTCB->usStackDepth = usStackDepth;
    pxNewTCB->pvStack = malloc(usStackDepth ? usStackDepth : 1);
    pxNewTCB->pxNext = NULL;
    pxNewTCB->eCurrentState = TASK_READY;
    pxNewTCB->suspended = 0;
    pxNewTCB->alive = 0;
    pthread_mutex_init(&pxNewTCB->suspend_mutex, NULL);
    pthread_cond_init(&pxNewTCB->suspend_cond, NULL);

    // Solo almacenar, no crear thread todavía
    _miAddTaskToReadyList(pxNewTCB);

    if (pxCreatedTask != NULL) {
        *pxCreatedTask = (TaskHandle_t)pxNewTCB;
    }

    printf("[FreeRTOS] ++ CREADA: %s (Pri: %u)\n", pcName, uxPriority);
    return pdTRUE;
}

void vTaskStartScheduler(void) {
    printf("[FreeRTOS] ** SCHEDULER INICIADO\n");
    schedulerRunning = 1;

    // Crear e iniciar todos los threads
    pthread_mutex_lock(&global_tcb_mutex);
    miTCB *pxCurrent = pxReadyTasksList;
    // Count tasks first to allocate array
    int taskCount = 0;
    for (miTCB *it = pxCurrent; it != NULL; it = it->pxNext) taskCount++;
    pthread_t *threads = calloc(taskCount, sizeof(pthread_t));
    int idx = 0;
    for (miTCB *it = pxReadyTasksList; it != NULL; it = it->pxNext) {
        if (pthread_create(&threads[idx], NULL, _miTaskWrapper, it) == 0) {
            it->xThreadId = threads[idx];
            idx++;
        } else {
            printf("[FreeRTOS] !! ERROR iniciando: %s\n", it->pcTaskName);
        }
    }
    pthread_mutex_unlock(&global_tcb_mutex);

    printf("[FreeRTOS] Esperando finalización de %d tareas...\n", idx);

    // Join created threads
    for (int i = 0; i < idx; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);

    printf("[FreeRTOS] Todas las tareas completadas\n");
    schedulerRunning = 0;
}

void vTaskDelay(const TickType_t xTicksToDelay) {
    // actualizar tick count (no exacto pero suficiente)
    if (xTicksToDelay != portMAX_DELAY) {
        xTickCount += xTicksToDelay;
        usleep(xTicksToDelay * 1000); // ms -> us
    } else {
        // Espera indefinida (simular)
        pause();
    }
    // punto cancelable
    pthread_testcancel();
}

void taskYIELD(void) {
    // permitir que otras threads se ejecuten y chequear suspensión/cancelación
    pthread_testcancel();
    sched_yield();
    // Check if this thread is suspended
    miTCB *self = _find_tcb_by_thread(pthread_self());
    if (self) {
        pthread_mutex_lock(&self->suspend_mutex);
        while (self->suspended) {
            self->eCurrentState = TASK_SUSPENDED;
            pthread_cond_wait(&self->suspend_cond, &self->suspend_mutex);
            self->eCurrentState = TASK_RUNNING;
        }
        pthread_mutex_unlock(&self->suspend_mutex);
    }
}

void vTaskSuspend(TaskHandle_t xTaskToSuspend) {
    if (xTaskToSuspend == NULL) return;
    miTCB *t = (miTCB*)xTaskToSuspend;
    pthread_mutex_lock(&t->suspend_mutex);
    t->suspended = 1;
    t->eCurrentState = TASK_SUSPENDED;
    pthread_mutex_unlock(&t->suspend_mutex);
    // Note: if thread is inside blocking call (vTaskDelay or taskYIELD) it will wait.
    printf("[FreeRTOS] -- SUSPENDIDO: %s\n", t->pcTaskName);
}

void vTaskResume(TaskHandle_t xTaskToResume) {
    if (xTaskToResume == NULL) return;
    miTCB *t = (miTCB*)xTaskToResume;
    pthread_mutex_lock(&t->suspend_mutex);
    if (t->suspended) {
        t->suspended = 0;
        t->eCurrentState = TASK_READY;
        pthread_cond_signal(&t->suspend_cond);
        printf("[FreeRTOS] ++ RESUMIDO: %s\n", t->pcTaskName);
    }
    pthread_mutex_unlock(&t->suspend_mutex);
}

void vTaskDelete(TaskHandle_t xTaskToDelete) {
    if (xTaskToDelete == NULL) return;
    miTCB *t = (miTCB*)xTaskToDelete;
    // try to cancel thread
    if (t->alive) {
        pthread_cancel(t->xThreadId);
    }
    // free resources (detach from list)
    pthread_mutex_lock(&global_tcb_mutex);
    miTCB **cur = &pxReadyTasksList;
    while (*cur) {
        if (*cur == t) {
            miTCB* tmp = *cur;
            *cur = tmp->pxNext;
            // cleanup
            pthread_mutex_unlock(&global_tcb_mutex);
            pthread_mutex_destroy(&tmp->suspend_mutex);
            pthread_cond_destroy(&tmp->suspend_cond);
            if (tmp->pvStack) free(tmp->pvStack);
            free(tmp);
            printf("[FreeRTOS] -- ELIMINADA tarea\n");
            return;
        }
        cur = &((*cur)->pxNext);
    }
    pthread_mutex_unlock(&global_tcb_mutex);
}

// ==================== QUEUE IMPLEMENTATION ====================

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize) {
    if (uxQueueLength == 0 || uxItemSize == 0) return NULL;
    miQueue *q = (miQueue*)malloc(sizeof(miQueue));
    if (!q) return NULL;
    q->item_size = uxItemSize;
    q->length = uxQueueLength;
    q->buffer = malloc((size_t)uxQueueLength * uxItemSize);
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return (QueueHandle_t)q;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    miQueue *q = (miQueue*)xQueue;
    if (!q || !pvItemToQueue) return pdFAIL;

    struct timespec now, timeout;
    int rc = 0;
    pthread_mutex_lock(&q->mutex);

    while (q->count == q->length) {
        if (xTicksToWait == 0) {
            pthread_mutex_unlock(&q->mutex);
            return pdFAIL; // full and no wait
        } else if (xTicksToWait == portMAX_DELAY) {
            pthread_cond_wait(&q->not_full, &q->mutex);
        } else {
            clock_gettime(CLOCK_REALTIME, &now);
            timeout = now;
            _timespec_add_ms(&timeout, xTicksToWait);
            rc = pthread_cond_timedwait(&q->not_full, &q->mutex, &timeout);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&q->mutex);
                return pdFAIL;
            }
        }
    }
    // copy item
    void *dest = (char*)q->buffer + ((size_t)q->tail * q->item_size);
    memcpy(dest, pvItemToQueue, q->item_size);
    q->tail = (q->tail + 1) % q->length;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    miQueue *q = (miQueue*)xQueue;
    if (!q || !pvBuffer) return pdFAIL;

    struct timespec now, timeout;
    int rc = 0;
    pthread_mutex_lock(&q->mutex);

    while (q->count == 0) {
        if (xTicksToWait == 0) {
            pthread_mutex_unlock(&q->mutex);
            return pdFAIL; // empty and no wait
        } else if (xTicksToWait == portMAX_DELAY) {
            pthread_cond_wait(&q->not_empty, &q->mutex);
        } else {
            clock_gettime(CLOCK_REALTIME, &now);
            timeout = now;
            _timespec_add_ms(&timeout, xTicksToWait);
            rc = pthread_cond_timedwait(&q->not_empty, &q->mutex, &timeout);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&q->mutex);
                return pdFAIL;
            }
        }
    }
    // copy item
    void *src = (char*)q->buffer + ((size_t)q->head * q->item_size);
    memcpy(pvBuffer, src, q->item_size);
    q->head = (q->head + 1) % q->length;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return pdTRUE;
}

UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    miQueue *q = (miQueue*)xQueue;
    if (!q) return 0;
    pthread_mutex_lock(&q->mutex);
    UBaseType_t c = q->count;
    pthread_mutex_unlock(&q->mutex);
    return c;
}

// ==================== SEMAFOROS (MUTEX) ====================

SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    pthread_mutex_t *m = malloc(sizeof(pthread_mutex_t));
    if (!m) return NULL;
    pthread_mutex_init(m, NULL);
    return (SemaphoreHandle_t)m;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
    pthread_mutex_t *m = (pthread_mutex_t*)xSemaphore;
    if (!m) return pdFAIL;
    if (xBlockTime == 0) {
        if (pthread_mutex_trylock(m) == 0) return pdTRUE;
        else return pdFAIL;
    } else if (xBlockTime == portMAX_DELAY) {
        pthread_mutex_lock(m);
        return pdTRUE;
    } else {
        // timed try using absolute time
        struct timespec now, timeout;
        clock_gettime(CLOCK_REALTIME, &now);
        timeout = now;
        _timespec_add_ms(&timeout, xBlockTime);
        int rc = pthread_mutex_timedlock(m, &timeout);
        if (rc == 0) return pdTRUE;
        else return pdFAIL;
    }
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    pthread_mutex_t *m = (pthread_mutex_t*)xSemaphore;
    if (!m) return pdFAIL;
    pthread_mutex_unlock(m);
    return pdTRUE;
}


