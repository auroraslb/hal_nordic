/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 *   This file contains implementation of the nRF 802.15.4 GPIOTE abstraction.
 *
 * This implementation is to be used with Zephyr RTOS.
 *
 */

#include <platform/gpiote/nrf_802154_gpiote.h>

#include <stdbool.h>
#include <stdint.h>

#include <sys/__assert.h>
#include <device.h>
#include <toolchain.h>
#include <drivers/gpio.h>

#include <hal/nrf_gpio.h>
#include <nrf_802154_sl_coex.h>
#include <nrf_802154_sl_utils.h>

const static struct device *dev;
static struct gpio_callback grant_cb;
static uint32_t pin_number = COEX_GPIO_PIN_INVALID;

static void gpiote_irq_handler(const struct device *gpiob, struct gpio_callback *cb,
			       uint32_t pins)
{
	ARG_UNUSED(gpiob);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	nrf_802154_wifi_coex_gpiote_irqhandler();
}

void nrf_802154_gpiote_init(void)
{
	switch (nrf_802154_wifi_coex_interface_type_id_get()) {
	case NRF_802154_WIFI_COEX_IF_NONE:
		return;

	case NRF_802154_WIFI_COEX_IF_3WIRE: {
		nrf_802154_wifi_coex_3wire_if_config_t cfg;

		nrf_802154_wifi_coex_cfg_3wire_get(&cfg);

		pin_number = cfg.grant_cfg.gpio_pin;
		__ASSERT_NO_MSG(pin_number != COEX_GPIO_PIN_INVALID);

		/* Convert to pin number relative to the pin's port. */
		uint32_t port_number = nrf_gpio_pin_port_number_extract(&pin_number);

		uint32_t pull_up_down = cfg.grant_cfg.active_high ?
						GPIO_PULL_UP :
						GPIO_PULL_DOWN;

#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
		dev = device_get_binding((port_number > 0) ?
						DT_LABEL(DT_NODELABEL(gpio1)) :
						DT_LABEL(DT_NODELABEL(gpio0)));
#else
		dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
#endif
		__ASSERT_NO_MSG(dev != NULL);

		gpio_pin_configure(dev, pin_number, GPIO_INPUT | pull_up_down);

		gpio_init_callback(&grant_cb, gpiote_irq_handler,
				BIT(pin_number));
		gpio_add_callback(dev, &grant_cb);

		gpio_pin_interrupt_configure(dev, pin_number,
				GPIO_INT_EDGE_BOTH);
		break;
	}

	default:
		__ASSERT_NO_MSG(false);
	}
}

void nrf_802154_gpiote_deinit(void)
{
	switch (nrf_802154_wifi_coex_interface_type_id_get()) {
	case NRF_802154_WIFI_COEX_IF_NONE:
		break;

	case NRF_802154_WIFI_COEX_IF_3WIRE:
		gpio_pin_interrupt_configure(dev, pin_number, GPIO_INT_DISABLE);
		gpio_remove_callback(dev, &grant_cb);
		pin_number = COEX_GPIO_PIN_INVALID;
		break;

	default:
		__ASSERT_NO_MSG(false);
	}
}