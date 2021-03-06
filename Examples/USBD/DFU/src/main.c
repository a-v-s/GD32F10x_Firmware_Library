/*!
    \file  main.c
    \brief USB device main routine

    \version 2014-12-26, V1.0.0, firmware for GD32F10x
    \version 2017-06-20, V2.0.0, firmware for GD32F10x
    \version 2018-07-31, V2.1.0, firmware for GD32F10x
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "usbd_std.h"
#include "dfu_core.h"

pAppFunction Application;
uint32_t AppAddress;

usbd_core_handle_struct  usb_device_dev = {
    .class_init = dfu_init,
    .class_deinit = dfu_deinit,
    .class_req_handler = dfu_req_handler,
    .class_data_handler = dfu_data_handler,
};

extern uint8_t usbd_serial_string[];

void rcu_config(void);
void gpio_config(void);
void nvic_config(void);
void serial_string_create(void);

/*!
    \brief      main routine will construct a DFU device
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    /* configure tamper key to run firmware */
    gd_eval_key_init(KEY_TAMPER, KEY_MODE_GPIO);

    /* tamper key must be pressed on GD321x0-EVAL when power on */
    if(0 == gd_eval_key_state_get(KEY_TAMPER)){
        /* test if user code is programmed starting from address 0x08004000 */
        if (0x20000000 == ((*(__IO uint32_t*)APP_LOADED_ADDR) & 0x2FFE0000)) {
            AppAddress = *(__IO uint32_t*) (APP_LOADED_ADDR + 4);
            Application = (pAppFunction) AppAddress;

            /* initialize user application's stack pointer */
            __set_MSP(*(__IO uint32_t*) APP_LOADED_ADDR);

            /* jump to user application */
            Application();
        }
    }

    /* system clocks configuration */
    rcu_config();

    /* GPIO configuration */
    gpio_config();

    serial_string_create();

    usb_device_dev.dev_desc = (uint8_t *)&device_descripter;
    usb_device_dev.config_desc = (uint8_t *)&configuration_descriptor;
    usb_device_dev.strings = usbd_strings;

    /* USB device configuration */
    usbd_core_init(&usb_device_dev);

    /* NVIC configuration */
    nvic_config();

    /* enabled USB pull-up */
    gpio_bit_set(USB_PULLUP, USB_PULLUP_PIN);

    /* now the usb device is connected */
    usb_device_dev.status = USBD_CONNECTED;

    while (1);
}

/*!
    \brief      configure the different system clocks
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rcu_config(void)
{
    /* enable USB pull-up pin clock */ 
    rcu_periph_clock_enable(RCC_AHBPeriph_GPIO_PULLUP);

    /* configure USB model clock from PLL clock */
    rcu_usb_clock_config(RCU_CKUSB_CKPLL_DIV2);

    /* enable USB APB1 clock */
    rcu_periph_clock_enable(RCU_USBD);
}

/*!
    \brief      configure the gpio peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void gpio_config(void)
{
    /* configure usb pull-up pin */
    gpio_init(USB_PULLUP, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USB_PULLUP_PIN);
}

/*!
    \brief      configure interrupt priority
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    /* 1 bit for pre-emption priority, 3 bits for subpriority */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);

    /* enable the USB low priority interrupt */
    nvic_irq_enable(USBD_LP_CAN0_RX0_IRQn, 1, 0);
}

/*!
    \brief      create the serial number string descriptor
    \param[in]  none
    \param[out] none
    \retval     none
*/
void  serial_string_create (void)
{
    uint32_t device_serial = *(uint32_t*)DEVICE_ID;

    if(0 != device_serial) {
        usbd_serial_string[2] = (uint8_t)device_serial;
        usbd_serial_string[3] = (uint8_t)(device_serial >> 8);
        usbd_serial_string[4] = (uint8_t)(device_serial >> 16);
        usbd_serial_string[5] = (uint8_t)(device_serial >> 24);
    }
}
