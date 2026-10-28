/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"




// Define task priorities
#define PRIORITY_HIGH   ( tskIDLE_PRIORITY + 3 )
#define PRIORITY_MED    ( tskIDLE_PRIORITY + 2 )
#define PRIORITY_LOW    ( tskIDLE_PRIORITY + 1 )
#define TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define configUSE_MUTEXES 1

SemaphoreHandle_t SharedMutex = NULL;

// Task handles for management
TaskHandle_t xHighHandle = NULL;
TaskHandle_t xMedHandle  = NULL;
TaskHandle_t xLowHandle  = NULL;

// Shared Binary Semaphore
SemaphoreHandle_t SharedSemaphore = NULL;

// --- Thread Definitions ---

// 1. Low Priority Thread
void LowPriorityTask(void *pvParameters) {
    printf("[LOW] Running. Attempting to acquire semaphore...\n");
    // Step 1: Low priority grabs the semaphore immediately
    // if (xSemaphoreTake(SharedSemaphore, portMAX_DELAY) == pdTRUE) {
    if (xSemaphoreTake(SharedMutex, portMAX_DELAY) == pdTRUE) {
        printf("[LOW] Acquired semaphore...\n");
        // Burn CPU cycles or spin to simulate heavy work inside critical section
        // We use a busy-loop instead of vTaskDelay so the CPU stays busy at this priority
        // This tells the SDK to loop inside an assembly block until 40 ticks pass
        busy_wait_at_least_cycles(30); // wait about 40 ticks (needs to be odd of high task)
        printf("[LOW] Busy work done finished. Releasing semaphore...\n");
       // xSemaphoreGive(SharedSemaphore);
        xSemaphoreGive(SharedMutex);
    }
    vTaskDelete(NULL);
}

// 2. Medium Priority Thread
void MedPriorityTask(void *pvParameters) {
    // Step 3: Start delayed relative to LOW, but before LOW finishes
    printf("[MED] Spawned. I do not care about the semaphore. Starting infinite computation...\n");
    vTaskDelay(pdMS_TO_TICKS(25));

    // Spin infinitely. Because MED is higher priority than LOW, LOW will never run again.
    while (1) {
        volatile uint32_t uiKeepAlive = 0;
        uiKeepAlive++;
    }
}

// 3. High Priority Thread
void HighPriorityTask(void *pvParameters) {
    // Step 2: Delay start slightly to guarantee LOW acquires the semaphore first
    printf("[HIGH] Created. Demanding shared semaphore...\n");
    vTaskDelay(pdMS_TO_TICKS(20));
    printf("[HIGH]: attempting to take shared semaphore...\n");
    // Attempt to take the semaphore. Will block because LOW holds it.
    // if (xSemaphoreTake(SharedSemaphore, portMAX_DELAY) == pdTRUE) {
    if (xSemaphoreTake(SharedMutex, portMAX_DELAY) == pdTRUE) {
        printf("[HIGH] Acquired semaphore.\n");
        // xSemaphoreGive(SharedSemaphore);
        xSemaphoreGive(SharedMutex);
    }
    vTaskDelete(NULL);
}

// --- Supervisor / Manager Task ---
void SupervisorTask(void *pvParameters) {
    printf("=== Supervisor: Initializing Scenario ===\n");

    // Create tasks in order to control initial execution flow
    xTaskCreate(LowPriorityTask,  "LowTask",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_LOW,  &xLowHandle);
    xTaskCreate(HighPriorityTask, "HighTask", configMINIMAL_STACK_SIZE, NULL, PRIORITY_HIGH, &xHighHandle);
    xTaskCreate(MedPriorityTask,  "MedTask",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_MED,  &xMedHandle);

    // Monitor execution (Supervisor spins while experiment runs)
    vTaskDelay(pdMS_TO_TICKS(10000));

    printf("\n=== Supervisor: Scenario Timeout Reached ===\n");

    vTaskDelete(NULL);
}

int main(void)
{
    stdio_init_all();
    sleep_ms(5000);
    TaskHandle_t main, side;
    // SharedSemaphore = xSemaphoreCreateCounting(1, 1); // semaphore
    SharedMutex = xSemaphoreCreateMutex(); // mutex
    xTaskCreate(SupervisorTask, "supervisor Task",
                TASK_STACK_SIZE, NULL, PRIORITY_HIGH, &main);
    vTaskStartScheduler();
	return 0;
}