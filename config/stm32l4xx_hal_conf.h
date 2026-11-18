#pragma once

/* HAL_MODULE_ENABLED and the module switches below control which HAL headers
 * and driver modules are available to this firmware. */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

#define HSE_VALUE                   8000000U
#define HSE_STARTUP_TIMEOUT         100U
#define MSI_VALUE                   4000000U
#define HSI_VALUE                   16000000U
#define HSI48_VALUE                 48000000U
#define LSI_VALUE                   32000U
#define LSE_VALUE                   32768U
#define LSE_STARTUP_TIMEOUT         5000U
#define EXTERNAL_SAI1_CLOCK_VALUE   48000U
#define EXTERNAL_SAI2_CLOCK_VALUE   48000U
#define EXTERNAL_CLOCK_VALUE        48000U

#define VDD_VALUE                   3300U
#define TICK_INT_PRIORITY           0x0FU
/* Keep HAL's RTOS integration disabled; FreeRTOS provides the scheduler and
 * main.cpp advances HAL time from the FreeRTOS tick hook. */
#define USE_RTOS                    0U
#define PREFETCH_ENABLE             1U
#define INSTRUCTION_CACHE_ENABLE    1U
#define DATA_CACHE_ENABLE           1U

#include "stm32l4xx_hal_rcc.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_dma.h"
#include "stm32l4xx_hal_cortex.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32l4xx_hal_i2c.h"
#include "stm32l4xx_hal_pwr.h"
#include "stm32l4xx_hal_tim.h"
#include "stm32l4xx_hal_uart.h"

#define assert_param(expr) ((void)0U)