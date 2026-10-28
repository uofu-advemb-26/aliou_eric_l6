// for mac os sleep:
#include <unistd.h>

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

TaskHandle_t supervisor = NULL;

// Shared Binary Semaphore
SemaphoreHandle_t SharedSemaphore = NULL;

// not being used
void setUp(void) {

}

// not being used
void tearDown(void) {
}


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
void Task1(void *pvParameters) {
    if (xSemaphoreTake(SharedMutex, portMAX_DELAY) == pdTRUE) {

        xSemaphoreGive(SharedMutex);
    }
    vTaskDelete(NULL);
}
// 3. High Priority Thread
void Task2(void *pvParameters) {
    if (xSemaphoreTake(SharedMutex, portMAX_DELAY) == pdTRUE) {

        xSemaphoreGive(SharedMutex);
    }
    vTaskDelete(NULL);
}

// --- Supervisor / Manager Task ---
void TestSame_Priority(void *pvParameters) {

    // Create tasks in order to control initial execution flow
    xTaskCreate(Task1,  "Task1",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_LOW,  &xLowHandle);
    xTaskCreate(Task2,  "Task2",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_LOW,  &xLowHandle);

    vTaskDelete(NULL);
}

void busy_busy(void)
{
    for (int i = 0; ; i++);
}

void busy_yield(void)
{
    for (int i = 0; ; i++) {
        taskYIELD();
    }
}

void main_task (__unused void *params)
{
    sleep_ms(5000); // Give time for TTY to attach
    printf("Start tests\n");

    UNITY_BEGIN();

        RUN_TEST(TestSame_Priority);

    sleep_ms(5000);

    UNITY_END();

    printf("Tests complete. System idling safely. Press Ctrl+A then K to exit screen.\n");

    // FIX: Trap the task here so FreeRTOS never attempts to drop to a hardware sleep state
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    // can only be used on a pico board
    stdio_init_all();

    // NEW FIX: Force the Pico to pause here until you open the 'screen' command on your Mac
    while (!stdio_usb_connected()) {
        sleep_ms(10); // Check every 10 milliseconds
    }

    printf("terminal screen connected.");

    const char *rtos_name;
    rtos_name = "FreeRTOS";
    TaskHandle_t main_task;
    // create and start main a test task:
    xTaskCreate(main_task, "MainTestThread",
                TASK_STACK_SIZE, NULL, PRIORITY_HIGH, &main_task);
    vTaskStartScheduler();
    return 0;
}