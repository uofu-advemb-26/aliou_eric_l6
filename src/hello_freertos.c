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
    if (xSemaphoreTake(SharedSemaphore, portMAX_DELAY) == pdTRUE) {
        printf("[LOW] Acquired semaphore. Simulating long critical section...\n");
        // Burn CPU cycles or spin to simulate heavy work inside critical section
        // We use a busy-loop instead of vTaskDelay so the CPU stays busy at this priority
        volatile uint32_t uiCount;
        for (uiCount = 0; uiCount < 10000000; uiCount++);
        printf("[LOW] Critical section finished. Releasing semaphore...\n");
        xSemaphoreGive(SharedSemaphore);
    }
    vTaskDelete(NULL);
}

// 2. Medium Priority Thread
void MedPriorityTask(void *pvParameters) {
    // Step 3: Start delayed relative to LOW, but before LOW finishes
    vTaskDelay(pdMS_TO_TICKS(50));
    printf("[MED] Spawned. I do not care about the semaphore. Starting infinite computation...\n");
    // Spin infinitely. Because MED is higher priority than LOW, LOW will never run again.
    while (1) {
        volatile uint32_t uiKeepAlive = 0;
        uiKeepAlive++;
    }
}

// 3. High Priority Thread
void HighPriorityTask(void *pvParameters) {
    // Step 2: Delay start slightly to guarantee LOW acquires the semaphore first
    vTaskDelay(pdMS_TO_TICKS(20));
    printf("[HIGH] Spawned. Demanding shared semaphore...\n");
    // Attempt to take the semaphore. Will block because LOW holds it.
    if (xSemaphoreTake(SharedSemaphore, portMAX_DELAY) == pdTRUE) {
        printf("[HIGH] SUCCESS! Acquired semaphore.\n");
        xSemaphoreGive(SharedSemaphore);
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
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("\n=== Supervisor: Scenario Timeout Reached ===\n");
    printf("[ANALYSIS] HighTask is trapped indefinitely. Priority Inversion achieved.\n");

    vTaskDelete(NULL);
}

int main(void)
{
    stdio_init_all();
    TaskHandle_t main, side;
    SharedSemaphore = xSemaphoreCreateCounting(1, 1);
    xTaskCreate(SupervisorTask, "supervisor Task",
                TASK_STACK_SIZE, NULL, PRIORITY_HIGH, &main);
    vTaskStartScheduler();
	return 0;
}