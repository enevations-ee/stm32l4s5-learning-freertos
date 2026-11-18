#include <cstdio>
#include <cstring>

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef NDEBUG
// OpenOCD uses this symbol to determine the priority range for FreeRTOS thread
// awareness; keep it in debug builds even when the linker would otherwise drop it.
extern "C" {
__attribute__((used)) int uxTopUsedPriority = configMAX_PRIORITIES - 1;
}
#endif

static UART_HandleTypeDef huart1{};

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
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    // Configure the board's green LD2 as a low-speed push-pull output.
    GPIO_InitTypeDef led{};
    led.Pin = GPIO_PIN_14;
    led.Mode = GPIO_MODE_OUTPUT_PP;
    led.Pull = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    // Route USART1 to the ST-LINK virtual COM port pins (AF7).
    GPIO_InitTypeDef uart_pins{};
    uart_pins.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    uart_pins.Mode = GPIO_MODE_AF_PP;
    uart_pins.Pull = GPIO_NOPULL;
    uart_pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    uart_pins.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &uart_pins);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart1);
}

// --- Application Task ---
static void blinky_task(void*) {
    char msg[64];
    uint32_t count = 0;

    // UART writes are synchronous; this simple example logs directly from its task.
    const char* banner = "\r\nHello Blinky from STM32L4S5!\r\n";
    HAL_UART_Transmit(&huart1, reinterpret_cast<const uint8_t*>(banner), std::strlen(banner), 100);

    for (;;) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        std::snprintf(msg, sizeof(msg), "Blink #%lu (uptime: %lu ms)\r\n", ++count, HAL_GetTick());
        HAL_UART_Transmit(&huart1, reinterpret_cast<const uint8_t*>(msg), std::strlen(msg), 100);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    HAL_Init();
    system_clock_config();
    gpio_and_uart_init();

    // FreeRTOS stack depths are in words (256 words here), not bytes.
    xTaskCreate(blinky_task, "Blinky", 256, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    vTaskStartScheduler();

    // The scheduler should not return; reaching here indicates startup failure.
    while (true) {}
}