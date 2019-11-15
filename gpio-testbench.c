/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <metal/cpu.h>
#include <metal/gpio.h>
#include <metal/interrupt.h>

/* This example assumes that GPIO pins 0 and 1 are connected together
 *
 * It then performs the following steps:
 *   1. Write HIGH to pin 0, read HIGH from pin 1
 *   2. Write LOW to pin 0, read LOW from pin 1
 *   3. Configure pin 1 for a rising interrupt, write HIGH to pin 0
 */

#define HIGH 1
#define LOW 0

#define OUTPUT_PIN 0
#define INPUT_PIN 1

int interrupt_count = 0;

void gpio_int_handler(int id, void *priv) {
	struct metal_gpio* gpio = (struct metal_gpio *)priv;

	metal_gpio_clear_interrupt(gpio, INPUT_PIN, METAL_GPIO_INT_RISING);
	metal_gpio_config_interrupt(gpio, INPUT_PIN, METAL_GPIO_INT_DISABLE);

	interrupt_count += 1;
}

int main() {
	int rc = 0;

	/* Initialize CPU interrupt controller */
	struct metal_cpu *cpu = metal_cpu_get(metal_cpu_get_current_hartid());
	if (cpu == NULL) {
		return 1;
	}
	struct metal_interrupt *cpu_int = metal_cpu_interrupt_controller(cpu);
	if (cpu_int == NULL) {
		return 2;
	}
	metal_interrupt_init(cpu_int);

	/* Initialize GPIO device and interrupt controller */
	struct metal_gpio *gpio = metal_gpio_get_device(0);
	if (gpio == NULL) {
		return 3;
	}
	struct metal_interrupt *gpio_int = metal_gpio_interrupt_controller(gpio);
	if (gpio_int == NULL) {
		return 4;
	}
	metal_interrupt_init(gpio_int);
	int gpio_id = metal_gpio_get_interrupt_id(gpio, INPUT_PIN);

	/* Set input and output pins */
	metal_gpio_enable_input(gpio, INPUT_PIN);
	metal_gpio_enable_output(gpio, OUTPUT_PIN);

	/* Test synchronous setting and getting */
	metal_gpio_set_pin(gpio, OUTPUT_PIN, HIGH);
	if (metal_gpio_get_input_pin(gpio, INPUT_PIN) != HIGH) {
		return 5;
	}

	metal_gpio_set_pin(gpio, 0, LOW);
	if (metal_gpio_get_input_pin(gpio, INPUT_PIN) != LOW) {
		return 6;
	}

	/* Configure a rising interrupt and register a handler */
	rc = metal_gpio_config_interrupt(gpio, INPUT_PIN, METAL_GPIO_INT_RISING);
	if (rc != 0) {
		return 7;
	}
	rc = metal_interrupt_register_handler(gpio_int, gpio_id, gpio_int_handler, gpio);
	if (rc != 0) {
		return 8;
	}

	/* Enable interrupts */
	rc = metal_interrupt_enable(gpio_int, gpio_id);
	if (rc != 0) {
		return 9;
	}
	rc = metal_interrupt_enable(cpu_int, 0);
	if (rc != 0) {
		return 10;
	}

	/* Trigger a rising interrupt */
	metal_gpio_set_pin(gpio, OUTPUT_PIN, HIGH);
	if (interrupt_count != 1) {
		return 11;
	}

	return 0;
}

