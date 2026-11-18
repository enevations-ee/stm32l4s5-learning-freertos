#include <cstdio>

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.hpp"
#include "uart.hpp"

#ifndef NDEBUG
// OpenOCD uses this symbol to determine the priority range for FreeRTOS thread
// awareness; keep it in debug builds even when the linker would otherwise drop it.
extern "C" {
__attribute__((used)) int uxTopUsedPriority = configMAX_PRIORITIES - 1;
}
#endif

static constexpr Led green_led(GPIOB_BASE, GPIO_PIN_14);
static constexpr Led wifi_ble_led(GPIOC_BASE, GPIO_PIN_9);
static Uart uart1(USART1, GPIOB_BASE, GPIO_PIN_6, GPIO_PIN_7,
                  GPIO_AF7_USART1, 115200);

// --- FreeRTOS Kernel Hooks ---
extern "C" {

// Once the scheduler starts, FreeRTOS owns SysTick; keep the HAL millisecond
// clock in sync by advancing it from each RTOS tick.
void vApplicationTickHook() {
    HAL_IncTick();
}

// Fail-stop hooks make allocation and stack failures visible during debugging.
void vApplicationMallocFailedHook() {
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

void vApplicationStackOverflowHook(TaskHandle_t, char*) {
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

} // extern "C"

// --- Hardware Initialization ---
static void system_clock_config() {
    RCC_OscInitTypeDef osc{};
    RCC_ClkInitTypeDef clk{};

    // Scale 1 supports the selected 80 MHz system clock.
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState = RCC_MSI_ON;
    osc.MSICalibrationValue = 0;
    osc.MSIClockRange = RCC_MSIRANGE_6; // 4 MHz
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 40; // 4 MHz MSI / 1 * 40 = 160 MHz VCO
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2; // SYSCLK = 160 MHz / 2 = 80 MHz
    HAL_RCC_OscConfig(&osc);

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4);
}

static void gpio_and_uart_init() {
    green_led.init();
    wifi_ble_led.init();
    uart1.init();
}

// --- Application Task ---
static void blinky_task(void*) {
    char msg[64];
    uint32_t count = 0;

    //set initial state of the LEDs
    green_led.off();
    

    // UART writes are synchronous; this simple example logs directly from its task.
    const char* banner = "\r\nHello Blinky from STM32L4S5!\r\n";
    uart1.write(banner);

    for (;;) {
        green_led.toggle();
        std::snprintf(msg, sizeof(msg), "Blink #%lu (uptime: %lu ms)\r\n", ++count, HAL_GetTick());
        uart1.write(msg);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void hello_task(void*) {
    char msg[64];
    uint32_t count = 0;

    //set initial state of the LEDs
    wifi_ble_led.on();

    // UART writes are synchronous; this simple example logs directly from its task.
    const char* banner = "\r\nHello from STM32L4S5!\r\n";
    uart1.write(banner);

    for (;;) {
        std::snprintf(msg, sizeof(msg), "Hello #%lu (uptime: %lu ms)\r\n", ++count, HAL_GetTick());
        uart1.write(msg);
        wifi_ble_led.toggle();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}   

int main() {
    HAL_Init();
    system_clock_config();
    gpio_and_uart_init();

    // FreeRTOS stack depths are in words (256 words here), not bytes.
    xTaskCreate(blinky_task, "Blinky", 256, nullptr, tskIDLE_PRIORITY + 2, nullptr);
    xTaskCreate(hello_task, "Hello", 256, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    vTaskStartScheduler();

    // The scheduler should not return; reaching here indicates startup failure.
    while (true) {}
}