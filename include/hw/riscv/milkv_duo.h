/*
 * Milk-V Duo series machine interface
 *
 * Copyright (c) 2025 Milk-V, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_MILKV_DUO_H
#define HW_MILKV_DUO_H

#include "hw/boards.h"
#include "hw/cpu/cluster.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/timer/sg200x_timer.h"
#include "hw/ssi/sg200x_spi.h"
#include "hw/gpio/sg200x_gpio.h"
#include "hw/gpio/gpio_backend.h"

#define TYPE_RISCV_DUO_SOC "riscv.milkv.duo.soc"
#define RISCV_DUO_SOC(obj) \
    OBJECT_CHECK(MilkvDuoSoCState, (obj), TYPE_RISCV_DUO_SOC)

typedef struct MilkvDuoSoCState
{
    /*< private >*/
    DeviceState parent_obj;

    /*< public >*/
    RISCVHartArrayState cpus;
    DeviceState *plic;

    SG200XSPIState spi1;
    SG200XGPIOState gpio;
    GPIOBACKENDState gpio_back;

    SG200XTimerState timer;

    uint32_t serial;
    char *cpu_type;
    bool little;
    uint32_t machine_type;

} MilkvDuoSoCState;

#define IRQ(name) (s->little? MILKV_DUO_L_##name : MILKV_DUO_B_##name)
#define TYPE_MILKV_DUO_MACHINE   MACHINE_TYPE_NAME("milkv_duo")

#define MILKV_DUO_MACHINE(obj) \
    OBJECT_CHECK(MilkvDuoState, (obj), TYPE_MILKV_DUO_MACHINE)


enum{
    MACHINE_TYPE_DUO = 0,
    MACHINE_TYPE_DUO256 = 0,
    MACHINE_TYPE_DUOS = 0,
};

typedef struct MilkvDuoState
{
    /*< private >*/
    MachineState parent_obj;

    /*< public >*/
    MilkvDuoSoCState soc;
    int fdt_size;

    uint32_t serial;

    uint32_t machine_type;

    bool little;

} MilkvDuoState;


enum {
    MILKV_DUO_DEV_MAILBOX,
    MILKV_DUO_DEV_SYSCTRL,
    MILKV_DUO_DEV_CLINT,
    MILKV_DUO_DEV_TOP_MISC,
    MILKV_DUO_DEV_PINMUX,
    MILKV_DUO_DEV_PLL,
    MILKV_DUO_DEV_RSTGEN,
    MILKV_DUO_DEV_WDT0,
    MILKV_DUO_DEV_WDT1,
    MILKV_DUO_DEV_WDT2,
    MILKV_DUO_DEV_GPIO0,
    MILKV_DUO_DEV_GPIO1,
    MILKV_DUO_DEV_GPIO2,
    MILKV_DUO_DEV_GPIO3,
    MILKV_DUO_DEV_PWM0,
    MILKV_DUO_DEV_PWM1,
    MILKV_DUO_DEV_PWM2,
    MILKV_DUO_DEV_PWM3,
    MILKV_DUO_DEV_TIMER,
    MILKV_DUO_DEV_I2C0,
    MILKV_DUO_DEV_I2C1,
    MILKV_DUO_DEV_I2C2,
    MILKV_DUO_DEV_I2C3,
    MILKV_DUO_DEV_I2C4,
    MILKV_DUO_DEV_UART0,
    MILKV_DUO_DEV_UART1,
    MILKV_DUO_DEV_UART2,
    MILKV_DUO_DEV_UART3,
    MILKV_DUO_DEV_UART4,
    MILKV_DUO_DEV_SPI0,
    MILKV_DUO_DEV_SPI1,
    MILKV_DUO_DEV_SPI2,
    MILKV_DUO_DEV_SPI3,
    MILKV_DUO_DEV_PLIC,
    MILKV_DUO_DEV_RTC_GPIO,
    MILKV_DUO_DEV_MROM,
    MILKV_DUO_DEV_DDR
};

enum {
    MILKV_DUO_B_UART0_IRQ   = 44,
    MILKV_DUO_B_UART1_IRQ   = 45,
    MILKV_DUO_B_UART2_IRQ   = 46,
    MILKV_DUO_B_UART3_IRQ   = 47,
    MILKV_DUO_B_UART4_IRQ   = 48,
    MILKV_DUO_B_SPI0_IRQ    = 54,
    MILKV_DUO_B_SPI1_IRQ    = 55,
    MILKV_DUO_B_SPI2_IRQ    = 56,
    MILKV_DUO_B_GPIO0_IRQ   = 60,
    MILKV_DUO_B_GPIO1_IRQ   = 61,
    MILKV_DUO_B_GPIO2_IRQ   = 62,
    MILKV_DUO_B_GPIO3_IRQ   = 63,
    MILKV_DUO_B_TIMER0_IRQ  = 79,
    MILKV_DUO_B_TIMER1_IRQ  = 80,
    MILKV_DUO_B_TIMER2_IRQ  = 81,
    MILKV_DUO_B_TIMER3_IRQ  = 82,
    MILKV_DUO_B_TIMER4_IRQ  = 83,
    MILKV_DUO_B_TIMER5_IRQ  = 84,
    MILKV_DUO_B_TIMER6_IRQ  = 85,
    MILKV_DUO_B_TIMER7_IRQ  = 86,
};

enum {
    MILKV_DUO_L_UART0_IRQ   = 30,
    MILKV_DUO_L_UART1_IRQ   = 31,
    MILKV_DUO_L_I2C0_IRQ    = 32,
    MILKV_DUO_L_I2C1_IRQ    = 33,
    MILKV_DUO_L_I2C2_IRQ    = 34,
    MILKV_DUO_L_I2C3_IRQ    = 35,
    MILKV_DUO_L_I2C4_IRQ    = 36,
    MILKV_DUO_L_SPI0_IRQ    = 37,
    MILKV_DUO_L_SPI1_IRQ    = 38,
    MILKV_DUO_L_GPIO0_IRQ   = 41,
    MILKV_DUO_L_GPIO1_IRQ   = 42,
    MILKV_DUO_L_GPIO2_IRQ   = 43,
    MILKV_DUO_L_GPIO3_IRQ   = 44,
    MILKV_DUO_L_TIMER0_IRQ  = 55,
    MILKV_DUO_L_TIMER1_IRQ  = 56,
    MILKV_DUO_L_TIMER2_IRQ  = 57,
    MILKV_DUO_L_TIMER3_IRQ  = 58,
    MILKV_DUO_L_TIMER4_IRQ  = 59,
    MILKV_DUO_L_TIMER5_IRQ  = 60,
    MILKV_DUO_L_TIMER6_IRQ  = 61,
    MILKV_DUO_L_TIMER7_IRQ  = 62,
};

#define MILKV_DUO_PLIC_HART_CONFIG "M"


#endif /* HW_MILKV_DUO_H */