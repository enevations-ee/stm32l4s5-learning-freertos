#pragma once

#include <cstdint>
#include "stm32l4xx_hal.h"

/**
 * @brief Thin C++ wrapper for controlling an STM32 GPIO-backed LED.
 *
 * A Led stores its GPIO port base address and HAL pin mask. Objects can be
 * created as constexpr values, for example:
 * @code
 * static constexpr Led green_led(GPIOB_BASE, GPIO_PIN_14);
 * @endcode
 *
 * Call init() after HAL_Init() and before using on(), off(), or toggle().
 * The init() method enables the port clock and configures the pin as a low-speed,
 * push-pull output with no pull resistor. It does not represent active-low
 * circuitry; on() writes GPIO_PIN_SET and off() writes GPIO_PIN_RESET.
 */
class Led {
public:
    /**
     * @brief Constructs an LED descriptor.
     *
     * @param port_base HAL GPIO peripheral base address, such as GPIOA_BASE.
     * @param pin STM32 HAL pin mask, such as GPIO_PIN_14. This parameter is
     *        a mask, not a zero-based pin number.
     */
    constexpr Led(uintptr_t port_base, uint16_t pin) noexcept
        : port_base_(port_base), pin_(pin) {}

    /**
     * @brief Enables the GPIO port clock and initializes the LED pin.
     *
     * Supports GPIOA through GPIOE. Unsupported port base addresses leave the
     * port clock unchanged but are otherwise passed to HAL_GPIO_Init().
     * HAL_Init() must have been called before using this method.
     */
    void init() const noexcept {
        enable_clock();

        GPIO_InitTypeDef gpio{};
        gpio.Pin = pin_;
        gpio.Mode = GPIO_MODE_OUTPUT_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(port(), &gpio);
    }

    /// @brief Drives the LED pin high.
    void on() const noexcept {
        HAL_GPIO_WritePin(port(), pin_, GPIO_PIN_SET);
    }

    /// @brief Drives the LED pin low.
    void off() const noexcept {
        HAL_GPIO_WritePin(port(), pin_, GPIO_PIN_RESET);
    }

    /// @brief Inverts the current output state of the LED pin.
    void toggle() const noexcept {
        HAL_GPIO_TogglePin(port(), pin_);
    }

private:
    uintptr_t port_base_;
    uint16_t pin_;

    GPIO_TypeDef* port() const noexcept {
        return reinterpret_cast<GPIO_TypeDef*>(port_base_);
    }

    void enable_clock() const noexcept {
        switch (port_base_) {
            case GPIOA_BASE: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
            case GPIOB_BASE: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
            case GPIOC_BASE: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
            case GPIOD_BASE: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
            case GPIOE_BASE: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
            default: break;
        }
    }
};