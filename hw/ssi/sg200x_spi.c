/*
 * Milk-V DUO Board SPI Controller
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

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qemu/fifo8.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/ssi/sg200x_spi.h"


#define FIFO_CAPACITY   8

static void sg200x_spi_reset(DeviceState *d)
{
    SG200XSPIState *s = SG200X_SPI(d);

    memset(s->regs, 0, sizeof(s->regs));

    // TODO:update
}

static uint64_t sg200x_spi_read(void *opaque, hwaddr addr, unsigned int size)
{
    uint64_t r =0;
    return r;
}

static void sg200x_spi_write(void *opaque, hwaddr addr,
                             uint64_t val64, unsigned int size)
{

}

static const MemoryRegionOps sg200x_spi_ops = {
    .read = sg200x_spi_read,
    .write = sg200x_spi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4}};

static void sg200x_spi_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    SG200XSPIState *s = SG200X_SPI(dev);
    int i;

    s->spi = ssi_create_bus(dev, "spi");
    sysbus_init_irq(sbd, &s->irq);

    s->cs_lines = g_new0(qemu_irq, s->num_cs);
    for (i = 0; i < s->num_cs; i++)
    {
        sysbus_init_irq(sbd, &s->cs_lines[i]);
    }

    memory_region_init_io(&s->mmio, OBJECT(s), &sg200x_spi_ops, s,
                          TYPE_SG200X_SPI, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);

    fifo8_create(&s->tx_fifo, FIFO_CAPACITY);
    fifo8_create(&s->rx_fifo, FIFO_CAPACITY);
}

static const Property sg200x_spi_properties[] = {
    DEFINE_PROP_UINT32("num-cs", SG200XSPIState, num_cs, 1),
};

static void sg200x_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, sg200x_spi_properties);
    device_class_set_legacy_reset(dc, sg200x_spi_reset);
    dc->realize = sg200x_spi_realize;
}

static const TypeInfo sg200x_spi_info = {
    .name = TYPE_SG200X_SPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SG200XSPIState),
    .class_init = sg200x_spi_class_init,
};

static void sg200x_spi_register_types(void)
{
    type_register_static(&sg200x_spi_info);
}

type_init(sg200x_spi_register_types)