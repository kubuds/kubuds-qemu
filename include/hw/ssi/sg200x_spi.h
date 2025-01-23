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

#ifndef HW_SG200X_SPI_H
#define HW_SG200X_SPI_H

#include "qemu/fifo8.h"
#include "hw/sysbus.h"

#define SG200X_SPI_REG_NUM (24)

#define TYPE_SG200X_SPI "sg200x.spi"
#define SG200X_SPI(obj) OBJECT_CHECK(SG200XSPIState, (obj), TYPE_SG200X_SPI)

typedef struct SG200XSPIState
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;

    uint32_t num_cs;
    qemu_irq *cs_lines;

    SSIBus *spi;

    Fifo8 tx_fifo;
    Fifo8 rx_fifo;

    uint32_t regs[SG200X_SPI_REG_NUM];

} SG200XSPIState;

#endif /* HW_SG200X_SPI_H */