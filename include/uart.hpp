#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "stm32l4xx_hal.h"

/**
 * @brief Thin C++ wrapper for an STM32 HAL UART peripheral.
 *
 * The wrapper owns the UART handle and configures the GPIO alternate-function
 * pins during init(). Call init() after HAL_Init() and before write().
 */
class Uart {
public:
    /**
     * @brief Constructs a UART descriptor.
     *
     * @param instance USART peripheral instance, such as USART1.
     * @param port_base GPIO port base address for TX and RX pins.
     * @param tx_pin STM32 HAL TX pin mask.
     * @param rx_pin STM32 HAL RX pin mask.
     * @param alternate GPIO alternate-function selection.
     * @param baud_rate UART baud rate.
     */
    constexpr Uart(USART_TypeDef* instance,
                   uintptr_t port_base,
                   uint16_t tx_pin,
                   uint16_t rx_pin,
                   uint8_t alternate,
                   uint32_t baud_rate) noexcept
        : instance_(instance),
          port_base_(port_base),
          tx_pin_(tx_pin),
          rx_pin_(rx_pin),
          alternate_(alternate),
          baud_rate_(baud_rate) {}

    /**
     * @brief Configures the GPIO pins and initializes the UART peripheral.
     */
    void init() noexcept {
        enable_peripheral_clock();
        enable_gpio_clock();

        GPIO_InitTypeDef uart_pins{};
        uart_pins.Pin = tx_pin_ | rx_pin_;
        uart_pins.Mode = GPIO_MODE_AF_PP;
        uart_pins.Pull = GPIO_NOPULL;
        uart_pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        uart_pins.Alternate = alternate_;
        HAL_GPIO_Init(port(), &uart_pins);

        handle_.Instance = instance_;
        handle_.Init.BaudRate = baud_rate_;
        handle_.Init.WordLength = UART_WORDLENGTH_8B;
        handle_.Init.StopBits = UART_STOPBITS_1;
        handle_.Init.Parity = UART_PARITY_NONE;
        handle_.Init.Mode = UART_MODE_TX_RX;
        handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        handle_.Init.OverSampling = UART_OVERSAMPLING_16;
        handle_.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
        handle_.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
        HAL_UART_Init(&handle_);
    }

    /**
     * @brief Transmits a binary buffer synchronously.
     *
     * @return HAL_UART_Transmit status.
     */
    HAL_StatusTypeDef write(const uint8_t* data,
                            std::size_t length,
                            uint32_t timeout = 100) noexcept {
        if (data == nullptr || length > UINT16_MAX) {
            return HAL_ERROR;
        }
        return HAL_UART_Transmit(&handle_, data,
                                 static_cast<uint16_t>(length), timeout);
    }

    /**
     * @brief Transmits a null-terminated string synchronously.
     *
     * @return HAL_UART_Transmit status.
     */
    HAL_StatusTypeDef write(const char* text,
                            uint32_t timeout = 100) noexcept {
        if (text == nullptr) {
            return HAL_ERROR;
        }
        return write(reinterpret_cast<const uint8_t*>(text),
                     std::strlen(text), timeout);
    }

private:
    USART_TypeDef* instance_;
    uintptr_t port_base_;
    uint16_t tx_pin_;
    uint16_t rx_pin_;
    uint8_t alternate_;
    uint32_t baud_rate_;
    UART_HandleTypeDef handle_{};

    GPIO_TypeDef* port() const noexcept {
        return reinterpret_cast<GPIO_TypeDef*>(port_base_);
    }

    void enable_gpio_clock() const noexcept {
        switch (port_base_) {
            case GPIOA_BASE: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
            case GPIOB_BASE: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
            case GPIOC_BASE: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
            case GPIOD_BASE: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
            case GPIOE_BASE: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
            default: break;
        }
    }

    void enable_peripheral_clock() const noexcept {
        if (instance_ == USART1) {
            __HAL_RCC_USART1_CLK_ENABLE();
        }
    }
};
