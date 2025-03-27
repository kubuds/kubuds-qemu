/*
 * GPIO backend connect to chardev
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

 #ifndef VIRT_GPIO_BACKEND_H
 #define VIRT_GPIO_BACKEND_H
 
 #include "hw/sysbus.h"
 #include "qom/object.h"
#include "qemu/timer.h"
 #include "qom/object.h"

 #define TYPE_GPIOBACKEND "gpio-backend"
 OBJECT_DECLARE_SIMPLE_TYPE(GPIOBACKENDState, GPIOBACKEND)
 #define GPIO_BACKEND_INTERNAL 100 /* 100ms */
 
 struct GPIOBACKENDState {
     SysBusDevice parent_obj;
 
     // QEMUTimer *timer;
     uint8_t in[10];
     uint8_t in_len;
     CharBackend chr;
     qemu_irq output[32];
 };
 
 #endif /* SG200X_GPIO_H */
