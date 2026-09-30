// test funcs
#include <thread_tests.h>
#include "unity_config.h"

// for mac os sleep:
#include <unistd.h>

// not being used
void setUp(void) {}

// not being used
void tearDown(void) {}

void main_task (__unused void *params)
{
    sleep_ms(5000); // Give time for TTY to attach
    printf("Start tests\n");
    // call this once
    hard_assert(cyw43_arch_init() == PICO_OK);

    UNITY_BEGIN();
    // RUN_TEST();
    // RUN_TEST();
    sleep_ms(5000);

    // safly kill
    cyw43_arch_deinit();
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
    // create and start main a test task:
    TaskHandle_t task;
    xTaskCreate(main_task, "MainTestThread",
                MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task);
    vTaskStartScheduler();
    return 0;
}