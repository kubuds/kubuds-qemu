/*
 * QEMU RISC-V Board Compatible with Milk-V DUO SDK
 *
 * Copyright (c) 2023 PLCT Lab.
 * Copyright (c) 2025 Milk-V, Inc.
 *
 * Provides a board compatible with the Milk-V DUO SDK:
 *
 * 0) UART
 * 1) CLINT (Core Level Interruptor)
 * 2) PLIC (Platform Level Interrupt Controller)
 * 4) GPIO (General Purpose Input/Output Controller)
 *
 * This board currently generates devicetree dynamically that indicates at least
 * two harts and up to five harts.
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

#include "qemu/osdep.h"
#include "qemu/cutils.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "hw/misc/unimp.h"
#include "target/riscv/cpu.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/riscv/milkv_duo.h"
#include "hw/riscv/boot.h"
#include "hw/char/milkv_uart.h"
#include "hw/intc/riscv_aclint.h"
#include "hw/intc/sifive_plic.h"
#include "chardev/char.h"
#include "hw/riscv/virt.h"
#include "system/device_tree.h"
#include "system/runstate.h"
#include "system/system.h"

//TODO：SPI I2C PWM

static const MemMapEntry milkv_duo_memmap[] = {
    [MILKV_DUO_DEV_MROM]     =     { 0x00001000,    0xF000 },
    [MILKV_DUO_DEV_MAILBOX]  =     { 0x01900000,    0x1000 },
    [MILKV_DUO_DEV_SYSCTRL]  =     { 0x01901000,    0x1000 },
    [MILKV_DUO_DEV_TOP_MISC] =     { 0x03000000,    0x1000 },
    [MILKV_DUO_DEV_PINMUX]   =     { 0x03001000,    0x1000 },
    [MILKV_DUO_DEV_PLL]      =     { 0x03002000,    0x1000 },
    [MILKV_DUO_DEV_RSTGEN]   =     { 0x03003000,    0x1000 },
    [MILKV_DUO_DEV_WDT0]     =     { 0x03010000,    0x1000 },
    [MILKV_DUO_DEV_WDT1]     =     { 0x03011000,    0x1000 },
    [MILKV_DUO_DEV_WDT2]     =     { 0x03012000,    0x1000 },
    [MILKV_DUO_DEV_GPIO0]    =     { 0x03020000,    0x1000 },
    [MILKV_DUO_DEV_GPIO1]    =     { 0x03021000,    0x1000 },
    [MILKV_DUO_DEV_GPIO2]    =     { 0x03022000,    0x1000 },
    [MILKV_DUO_DEV_GPIO3]    =     { 0x03023000,    0x1000 },
    [MILKV_DUO_DEV_PWM0]     =     { 0x03060000,    0x1000 },
    [MILKV_DUO_DEV_PWM1]     =     { 0x03061000,    0x1000 },
    [MILKV_DUO_DEV_PWM2]     =     { 0x03062000,    0x1000 },
    [MILKV_DUO_DEV_PWM3]     =     { 0x03063000,    0x1000 },
    [MILKV_DUO_DEV_TIMER]    =     { 0x030A0000,   0x10000 },
    [MILKV_DUO_DEV_I2C0]     =     { 0x04000000,   0x10000 },
    [MILKV_DUO_DEV_I2C1]     =     { 0x04010000,   0x10000 },
    [MILKV_DUO_DEV_I2C2]     =     { 0x04020000,   0x10000 },
    [MILKV_DUO_DEV_I2C3]     =     { 0x04030000,   0x10000 },
    [MILKV_DUO_DEV_I2C4]     =     { 0x04040000,   0x10000 },
    [MILKV_DUO_DEV_UART0]    =     { 0x04140000,   0x10000 },
    [MILKV_DUO_DEV_UART1]    =     { 0x04150000,   0x10000 },
    [MILKV_DUO_DEV_UART2]    =     { 0x04160000,   0x10000 },
    [MILKV_DUO_DEV_UART3]    =     { 0x04170000,   0x10000 },
    [MILKV_DUO_DEV_SPI0]     =     { 0x04180000,   0x10000 },
    [MILKV_DUO_DEV_SPI1]     =     { 0x04190000,   0x10000 },
    [MILKV_DUO_DEV_SPI2]     =     { 0x041A0000,   0x10000 },
    [MILKV_DUO_DEV_SPI3]     =     { 0x041B0000,   0x10000 },
    [MILKV_DUO_DEV_UART4]    =     { 0x041C0000,   0x10000 },
    [MILKV_DUO_DEV_RTC_GPIO] =     { 0x05021000,    0x1000 },
    [MILKV_DUO_DEV_PLIC]     =     { 0x70000000, 0x4000000 },
    [MILKV_DUO_DEV_CLINT]    =     { 0x74000000,   0x10000 },
    [MILKV_DUO_DEV_DDR]      =     { 0x80000000,         0 },
};

static void milkv_duo_machine_init(MachineState *machine)
{
    const MemMapEntry *memmap = milkv_duo_memmap;
    //TODO:FIXME
    MilkvDuoState *s = MILKV_DUO_MACHINE(machine);
    MemoryRegion *sys_mem = get_system_memory();

    hwaddr start_addr = memmap[MILKV_DUO_DEV_DDR].base;
    target_ulong firmware_end_addr, kernel_start_addr;
    const char *firmware_name;
    uint32_t kernel_entry_hi32 = 0x00000000;

    MachineClass *mc = MACHINE_GET_CLASS(machine);
    uint64_t kernel_entry;

    RISCVBootInfo boot_info;
    switch (s->machine_type) {
    case 0:
        mc->default_ram_size = 128 * MiB;
        break;
    case 1:
        mc->default_ram_size = 256 * MiB;
        break;
    case 2:
        mc->default_ram_size = 512 * MiB;
        break;
    default:
        break;
    }

    if (machine->ram_size != mc->default_ram_size) {
        char *sz = size_to_str(mc->default_ram_size);
        error_report("Invalid RAM size, should be %s", sz);
        g_free(sz);
        exit(EXIT_FAILURE);
    }

    /* Initialize SoC */
    object_initialize_child(OBJECT(machine), "soc", &s->soc, TYPE_RISCV_DUO_SOC);
    object_property_set_bool(OBJECT(&s->soc), "little", s->little,
                            &error_abort);
    if (s->little) {
        object_property_set_str(OBJECT(&s->soc), "cpu-type", TYPE_RISCV_CPU_THEAD_C906M,
                                &error_abort);
    } else {
        object_property_set_str(OBJECT(&s->soc), "cpu-type", machine->cpu_type,
                                &error_abort);
    }
    object_property_set_uint(OBJECT(&s->soc), "machine-type", s->machine_type,
                             &error_abort);
    qdev_realize(DEVICE(&s->soc), NULL, &error_fatal);

    /* Data Tightly Integrated Memory */
    memory_region_add_subregion(sys_mem, memmap[MILKV_DUO_DEV_DDR].base, machine->ram);

    firmware_name = riscv_default_firmware_name(&s->soc.cpus);
    firmware_end_addr = riscv_find_and_load_firmware(machine, firmware_name,
                                                     &start_addr, NULL);

    riscv_boot_info_init(&boot_info, &s->soc.cpus);
    if (machine->kernel_filename) {
        kernel_start_addr = riscv_calc_kernel_start_addr(&boot_info,
                                                         firmware_end_addr);

        riscv_load_kernel(machine, &boot_info, kernel_start_addr,
                        true, NULL);
        kernel_entry = boot_info.image_low_addr;

    } else {
        kernel_entry = 0;
    }

    kernel_entry_hi32 = (uint64_t)kernel_entry >> 32;

    /* reset vector */
    uint32_t reset_vec[12] = {
        0x00000297,                    /* 1:  auipc  t0, %pcrel_hi(fw_dyn) */
        0x02c28613,                    /*     addi   a2, t0, %pcrel_lo(1b) */
        0xf1402573,                    /*     csrr   a0, mhartid  */
        0,
        0,
        0x00028067,                    /*     jr     t0 */
        kernel_entry,                    /* start: .dword */
        kernel_entry_hi32,
        0x00000000,                 /* fdt_laddr: .dword */
        0x00000000,
        0x00000000,
                                       /* fw_dyn: */
    };

    reset_vec[3] = 0x0202b583;     /*     ld     a1, 32(t0) */
    reset_vec[4] = 0x0182b283;     /*     ld     t0, 24(t0) */

    /* copy in the reset vector in little_endian byte order */
    for (int i = 0; i < ARRAY_SIZE(reset_vec); i++) {
        reset_vec[i] = cpu_to_le32(reset_vec[i]);
    }
    rom_add_blob_fixed_as("mrom.reset", reset_vec, sizeof(reset_vec),
                          memmap[MILKV_DUO_DEV_MROM].base, &address_space_memory);

    riscv_rom_copy_firmware_info(machine, &s->soc.cpus,
                                 memmap[MILKV_DUO_DEV_MROM].base,
                                 memmap[MILKV_DUO_DEV_MROM].size,
                                 sizeof(reset_vec), kernel_entry);
}

static void milkv_duo_machine_instance_init(Object *obj)
{
    MilkvDuoState *s = MILKV_DUO_MACHINE(obj);
    s->machine_type = MACHINE_TYPE_DUO;

    object_property_add_uint32_ptr(obj, "machine-type", &s->machine_type,
                                   OBJ_PROP_FLAG_READWRITE);
    object_property_set_description(obj, "machine-type",
                                    "DUO type: Duo(0),Duo256(1),DuoS(2)");
}

static bool milkv_duo_machine_get_little(Object *obj, Error **errp)
{
    MilkvDuoState *s = MILKV_DUO_MACHINE(obj);

    return s->little;
}

static void milkv_duo_machine_set_little(Object *obj, bool value, Error **errp)
{
    MilkvDuoState *s = MILKV_DUO_MACHINE(obj);

    s->little = value;
}

static void milkv_duo_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "MILK-V DUO Board";
    mc->init = milkv_duo_machine_init;
    mc->max_cpus = 1;
    mc->min_cpus = 1;
    mc->default_cpu_type = TYPE_RISCV_CPU_THEAD_C906V;
    mc->default_cpus = mc->min_cpus;
    mc->default_ram_id = "riscv.milkv.duo.ram";

    object_class_property_add_bool(oc, "little", milkv_duo_machine_get_little,
                                   milkv_duo_machine_set_little);
    object_class_property_set_description(oc, "little",
                                          "1: c906-little Select or 0: c906");
}

static const TypeInfo milkv_duo_machine_typeinfo = {
    .name = TYPE_MILKV_DUO_MACHINE,
    .parent = TYPE_MACHINE,
    .class_init = milkv_duo_machine_class_init,
    .instance_init = milkv_duo_machine_instance_init,
    .instance_size = sizeof(MilkvDuoState),
};

static void milkv_duo_machine_init_register_types(void)
{
    type_register_static(&milkv_duo_machine_typeinfo);
}

type_init(milkv_duo_machine_init_register_types)


//init SOC
static void milkv_duo_soc_instance_init(Object *obj)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    MilkvDuoSoCState *s = RISCV_DUO_SOC(obj);

    object_initialize_child(obj, "cpus", &s->cpus, TYPE_RISCV_HART_ARRAY);
    qdev_prop_set_uint32(DEVICE(&s->cpus), "num-harts", ms->smp.cpus);
    qdev_prop_set_uint32(DEVICE(&s->cpus), "hartid-base", 0);
    qdev_prop_set_uint64(DEVICE(&s->cpus), "resetvec", 0x1000);
    // Initialize the 'timer' child object within the SoC state.
    object_initialize_child(obj, "timer", &s->timer, TYPE_SG200X_TIMER);

    // Initialize peripherals
    object_initialize_child(obj, "gpio", &s->gpio, TYPE_SG200X_GPIO);
    object_initialize_child(obj, "gpio-backend", &s->gpio_back, TYPE_GPIOBACKEND);
    object_initialize_child(obj, "spi1", &s->spi1, TYPE_SG200X_SPI);
}

static void milkv_duo_soc_realize(DeviceState *dev, Error **errp)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    MilkvDuoSoCState *s = RISCV_DUO_SOC(dev);
    const MemMapEntry *memmap = milkv_duo_memmap;
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *mask_rom = g_new(MemoryRegion, 1);

    int j;

    object_property_set_str(OBJECT(&s->cpus), "cpu-type", s->cpu_type, &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&s->cpus), &error_fatal);
    /* boot rom */
    memory_region_init_rom(mask_rom, OBJECT(dev), "riscv.milkv.duo.mrom",
                           memmap[MILKV_DUO_DEV_MROM].size, &error_fatal);
    memory_region_add_subregion(system_memory, memmap[MILKV_DUO_DEV_MROM].base,
                                mask_rom);

    s->plic = sifive_plic_create(memmap[MILKV_DUO_DEV_PLIC].base,
        (char *)MILKV_DUO_PLIC_HART_CONFIG, 1, 0,
        VIRT_IRQCHIP_NUM_SOURCES,
        ((1U << VIRT_IRQCHIP_NUM_PRIO_BITS) - 1),
        VIRT_PLIC_PRIORITY_BASE,
        VIRT_PLIC_PENDING_BASE,
        VIRT_PLIC_ENABLE_BASE,
        VIRT_PLIC_ENABLE_STRIDE,
        VIRT_PLIC_CONTEXT_BASE,
        VIRT_PLIC_CONTEXT_STRIDE,
        memmap[MILKV_DUO_DEV_PLIC].size);

    duo_uart_create(system_memory, memmap[MILKV_DUO_DEV_UART0].base,
        serial_hd(0), qdev_get_gpio_in(DEVICE(s->plic), IRQ(UART0_IRQ)));

    duo_uart_create(system_memory, memmap[MILKV_DUO_DEV_UART1].base,
        serial_hd(1), qdev_get_gpio_in(DEVICE(s->plic), IRQ(UART1_IRQ)));

    riscv_aclint_swi_create(memmap[MILKV_DUO_DEV_CLINT].base, 0,
                            ms->smp.cpus, false);
    riscv_aclint_mtimer_create(memmap[MILKV_DUO_DEV_CLINT].base +
            RISCV_ACLINT_SWI_SIZE,
        RISCV_ACLINT_DEFAULT_MTIMER_SIZE, 0, ms->smp.cpus,
        RISCV_ACLINT_DEFAULT_MTIMECMP, RISCV_ACLINT_DEFAULT_MTIME,
        RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ, true);
    
    /* TIMER */
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->timer), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0,
                    memmap[MILKV_DUO_DEV_TIMER].base);

    /* Connect TIMER interrupts to the PLIC */
    for (j = 0; j < SG200X_TIMER_MAX_NUM; j++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), j,
                           qdev_get_gpio_in(DEVICE(s->plic),
                                            IRQ(TIMER0_IRQ) + j));
    }

    /*GPIO*/
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->gpio), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, memmap[MILKV_DUO_DEV_GPIO0].base);

    qdev_pass_gpios(DEVICE(&s->gpio), dev, NULL);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->gpio), 0,
                    qdev_get_gpio_in(DEVICE(s->plic),
                                        IRQ(GPIO0_IRQ)));

    /*GPIO BACKEND*/
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->gpio_back), errp)) {
        return;
    }

    qdev_pass_gpios(DEVICE(&s->gpio_back), dev, NULL);
    for (int i = 0; i < 32; i++) {
        qdev_connect_gpio_out_named(DEVICE(&s->gpio_back), "out", i,
                        qdev_get_gpio_in_named(DEVICE(&s->gpio),"in", i));
        qdev_connect_gpio_out_named(DEVICE(&s->gpio), "out", i,
                        qdev_get_gpio_in_named(DEVICE(&s->gpio_back),"in", i));
    }

    /*SPI*/
    sysbus_realize(SYS_BUS_DEVICE(&s->spi1), errp);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi1), 0,
                    memmap[MILKV_DUO_DEV_SPI1].base);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->spi1), 0,
                       qdev_get_gpio_in(DEVICE(s->plic), IRQ(SPI1_IRQ)));

    create_unimplemented_device("riscv.milkv.duo.mailbox",
        memmap[MILKV_DUO_DEV_MAILBOX].base, memmap[MILKV_DUO_DEV_MAILBOX].size);
    create_unimplemented_device("riscv.milkv.duo.sysctrl",
        memmap[MILKV_DUO_DEV_SYSCTRL].base, memmap[MILKV_DUO_DEV_SYSCTRL].size);
    create_unimplemented_device("riscv.milkv.duo.topmisc",
        memmap[MILKV_DUO_DEV_TOP_MISC].base, memmap[MILKV_DUO_DEV_TOP_MISC].size);
    create_unimplemented_device("riscv.milkv.duo.pinmux",
        memmap[MILKV_DUO_DEV_PINMUX].base, memmap[MILKV_DUO_DEV_PINMUX].size);
    create_unimplemented_device("riscv.milkv.duo.pll",
        memmap[MILKV_DUO_DEV_PLL].base, memmap[MILKV_DUO_DEV_PLL].size);
    create_unimplemented_device("riscv.milkv.duo.rstgen",
        memmap[MILKV_DUO_DEV_RSTGEN].base, memmap[MILKV_DUO_DEV_RSTGEN].size);
    create_unimplemented_device("riscv.milkv.duo.rtcgpio",
        memmap[MILKV_DUO_DEV_RTC_GPIO].base, memmap[MILKV_DUO_DEV_RTC_GPIO].size);
}

static const Property milkv_duo_soc_props[] = {
    DEFINE_PROP_STRING("cpu-type", MilkvDuoSoCState, cpu_type),
    DEFINE_PROP_BOOL("little", MilkvDuoSoCState, little, false),
    DEFINE_PROP_UINT32("machine-type", MilkvDuoSoCState, machine_type, 0),
};

static void milkv_duo_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    device_class_set_props(dc, milkv_duo_soc_props);
    dc->realize = milkv_duo_soc_realize;
    /* Reason: Uses serial_hds in realize function, thus can't be used twice */
    dc->user_creatable = false;
}

static const TypeInfo milkv_duo_soc_type_info = {
    .name = TYPE_RISCV_DUO_SOC,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(MilkvDuoSoCState),
    .instance_init = milkv_duo_soc_instance_init,
    .class_init = milkv_duo_soc_class_init,
};

static void milkv_duo_soc_register_types(void)
{
    type_register_static(&milkv_duo_soc_type_info);
}

type_init(milkv_duo_soc_register_types)
