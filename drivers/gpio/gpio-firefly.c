/*
 * Driver for pwm demo on Firefly board.
 *
 * Copyright (C) 2016, Zhongshan T-chip Intelligent Technology Co.,ltd.
 * Copyright 2006  JC.Lin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define FIREFLY_HIGH_KERNEL_VERSION 1

struct firefly_gpio_info {
#ifdef FIREFLY_HIGH_KERNEL_VERSION
	struct gpio_desc* firefly_gpiod;
#else
	int firefly_gpio;
#endif
    int gpio_enable_value;
    int firefly_irq_gpio;
    int firefly_irq;
    int firefly_irq_mode;
};

static irqreturn_t firefly_gpio_irq(int irq, void *dev_id)
{
    printk("Enter firefly gpio irq test program!\n");
        return IRQ_HANDLED;
}

static int firefly_gpio_probe(struct platform_device *pdev)
{
    int ret;
    int gpio;
	int time;
    enum of_gpio_flags flag;
    struct firefly_gpio_info *gpio_info;
    struct device_node *firefly_gpio_node = pdev->dev.of_node;

    printk("Firefly GPIO Test Program Probe\n");
	gpio_info = devm_kzalloc(&pdev->dev,sizeof(struct firefly_gpio_info *), GFP_KERNEL);
    if (!gpio_info) {
        dev_err(&pdev->dev, "devm_kzalloc failed!\n");
                return -ENOMEM;
    }

#ifdef FIREFLY_HIGH_KERNEL_VERSION
	gpio_info->firefly_gpiod = devm_gpiod_get_optional(&pdev->dev, "firefly", GPIOD_OUT_LOW);
    if (IS_ERR(gpio_info->firefly_gpiod)) {
        dev_err(&pdev->dev, "failed to get firefly gpio\n");
		return -ENODEV; 
    }
    ret=of_property_read_u32(firefly_gpio_node,"firefly-mdelay",&time);
	if(!ret && (time > 0)){
		printk("firefly gpio delay %d ms\n",time);
		msleep(time);
	}
	printk("firefly gpio out high\n");
	gpiod_set_value_cansleep(gpio_info->firefly_gpiod, 1);
#else
    gpio = of_get_named_gpio_flags(firefly_gpio_node, "firefly-gpio", 0, &flag);
    if (!gpio_is_valid(gpio)) {
        dev_err(&pdev->dev, "firefly-gpio: %d is invalid\n", gpio);
        return -ENODEV;
    }
    if (gpio_request(gpio, "firefly-gpio")) {
        dev_err(&pdev->dev, "firefly-gpio: %d request failed!\n", gpio);
        gpio_free(gpio);
        return -ENODEV;
    }
    gpio_info->firefly_gpio = gpio;
    gpio_info->gpio_enable_value = (flag == OF_GPIO_ACTIVE_LOW) ? 0:1;
    gpio_direction_output(gpio_info->firefly_gpio, gpio_info->gpio_enable_value);
    printk("Firefly gpio putout\n");
#endif

    gpio = of_get_named_gpio_flags(firefly_gpio_node, "firefly-irq-gpio", 0, &flag);
     if (!gpio_is_valid(gpio)) {
        printk("firefly-irq-gpio: %d is invalid\n", gpio);
        return 0;
    }

    gpio_info->firefly_irq_gpio = gpio;
    gpio_info->firefly_irq_mode = flag;
    gpio_info->firefly_irq = gpio_to_irq(gpio_info->firefly_irq_gpio);
    if (gpio_info->firefly_irq) {
        if (gpio_request(gpio, "firefly-irq-gpio")) {
            dev_err(&pdev->dev, "firefly-irq-gpio: %d request failed!\n", gpio);
            gpio_free(gpio);
            return IRQ_NONE;
        }

            ret = request_irq(gpio_info->firefly_irq, firefly_gpio_irq,
                flag, "firefly-gpio", gpio_info);
            if (ret != 0) {
            free_irq(gpio_info->firefly_irq, gpio_info);
                    dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
        }
    }
    return 0;
}

static struct of_device_id firefly_match_table[] = {
        { .compatible = "firefly,rk3588-gpio",},
        {},
};

static struct platform_driver firefly_gpio_driver = {
        .driver = {
                .name = "firefly-gpio",
                .owner = THIS_MODULE,
                .of_match_table = firefly_match_table,
        },
        .probe = firefly_gpio_probe,
};

static int firefly_gpio_init(void)
{
        return platform_driver_register(&firefly_gpio_driver);
}
late_initcall(firefly_gpio_init);

static void firefly_gpio_exit(void)
{
        platform_driver_unregister(&firefly_gpio_driver);
}
module_exit(firefly_gpio_exit);

MODULE_AUTHOR("linjc <service@t-firefly.com>");
MODULE_DESCRIPTION("Firefly GPIO driver");
MODULE_ALIAS("platform:firefly-gpio");
MODULE_LICENSE("GPL");
