/*
 * GPIO backend
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

 #include "qemu/osdep.h"
 #include "qapi/error.h"
 #include "qemu/log.h"
 #include "hw/irq.h"
 #include "hw/sysbus.h"
 #include "migration/vmstate.h"
 #include "chardev/char.h"
 #include "chardev/char-fe.h"
 #include "qemu/module.h"
 #include "hw/gpio/gpio_backend.h"

 static const VMStateDescription vmstate_gpio_backend = {
     .name = "gpio-backend",
     .version_id = 1,
     .minimum_version_id = 1,
     .fields = (const VMStateField[]) {
         VMSTATE_UINT8_ARRAY(in, GPIOBACKENDState, 10),
         VMSTATE_UINT8(in_len, GPIOBACKENDState),
         VMSTATE_END_OF_LIST()
     }
 };
 
 static void gpio_backend_reset(DeviceState *dev)
 {
    GPIOBACKENDState *s = GPIOBACKEND(dev);
    s->in_len = 0;
 }
 
 static void gpio_backend_set(void *opaque, int line, int level)
 {
    GPIOBACKENDState *s = (GPIOBACKENDState *)opaque;
    uint8_t buf[10];
    sprintf((char*)buf, "%d: %d\n", line, level);
    qemu_chr_fe_write(&s->chr, buf, sizeof(buf)); 
 }

static void gpio_backend_rx(void *opaque, const uint8_t *buf, int size)
{
    GPIOBACKENDState *s = opaque;
    uint8_t line, value, ch;

    while (size > 0) {
        ch = *buf;
        if (ch != ' ' && ch != 10) 
            s->in[s->in_len++] = ch;

        if (ch == '#') {
            sscanf((char *)s->in, "%hhd:%hhd#", &line, &value);
            if (line < 32 && value <= 1)
                qemu_set_irq(s->output[line], value);
            else {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: receive bad input: addr=0x%x v=0x%x\n",
                    __func__, (int)line, (int)value);
            }
            s->in_len = 0;
        }
        size--;
        buf++;
    }
}

static int gpio_backend_can_rx(void *opaque)
{
    GPIOBACKENDState *s = opaque;
    return s->in_len < 10;
}

static void gpio_backend_event(void *opaque, QEMUChrEvent event)
{
}

static int gpio_backend_be_change(void *opaque)
{
    GPIOBACKENDState *s = opaque;

    qemu_chr_fe_set_handlers(&s->chr, gpio_backend_can_rx, gpio_backend_rx,
                             gpio_backend_event, gpio_backend_be_change, s,
                             NULL, true);

    return 0;
}

 static void gpio_backend_realize(DeviceState *dev, Error **errp)
 {
     GPIOBACKENDState *s = GPIOBACKEND(dev);
     // SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    
     qdev_init_gpio_out_named(DEVICE(s), s->output, "out", 32);
     qdev_init_gpio_in_named(dev, gpio_backend_set, "in", 32);
     Chardev* chr = qemu_chr_find("gpio-back");
    if (!chr) {
        qemu_log_mask(LOG_GUEST_ERROR, "chardev gpio-back not found");
        return;
    }
    qemu_chr_fe_init(&s->chr, chr, errp);
     // s->timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, gpio_backend_timer_expired, s);
    qemu_chr_fe_set_handlers(&s->chr, gpio_backend_can_rx, gpio_backend_rx,
        gpio_backend_event, gpio_backend_be_change, s,
        NULL, true);
 }
 
 static void gpio_backend_class_init(ObjectClass *klass, void *data)
 {
     DeviceClass *dc = DEVICE_CLASS(klass);
 
     dc->realize = gpio_backend_realize;
     dc->vmsd = &vmstate_gpio_backend;
     device_class_set_legacy_reset(dc, gpio_backend_reset);
 }
 
 static const TypeInfo gpio_backend_info = {
     .name          = TYPE_GPIOBACKEND,
     .parent        = TYPE_SYS_BUS_DEVICE,
     .instance_size = sizeof(GPIOBACKENDState),
     .class_init    = gpio_backend_class_init,
 };
 
 static void gpio_backend_register_types(void)
 {
     type_register_static(&gpio_backend_info);
 }
 
 type_init(gpio_backend_register_types)
