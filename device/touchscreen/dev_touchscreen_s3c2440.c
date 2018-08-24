/*
	dev_touchscreen_s3c2440.c
	ww
*/


#include <stdint.h>
#include <stdlib.h>
#include "skyeye_config.h"
#include "skyeye_device.h"

/* Pen_buffer: defined in skyeye_touchscreen.c */
extern unsigned int Pen_buffer[8];

struct ts_s3c2440_io {
	uint32_t ts_buffer[0x18];
};

static struct device_default_value s3c2440_ts_def[] = {
	/* name			base		size	interrupt array */
	{"s3c2410x",	0x58000000,	0x18,	{31, 0, 0, 0}},
	{"s3c2440",		0x58000000,	0x18,	{31, 0, 0, 0}},
	{NULL},
};

#if 0
static void ts_s3c2440_set_update_intr(struct device_desc *dev)
{
	struct device_interrupt *intr = &dev->intr;
	struct machine_config *mc = (struct machine_config*)dev->mach;
//	struct ts_s3c2440_io *io = (struct ts_s3c2440_io*)dev->data;

	if (mc->mach_set_intr == NULL) return;
	mc->mach_set_intr(intr->interrupts[0]);

	if (mc->mach_update_intr != NULL) mc->mach_update_intr(mc);
}
#endif

static void ts_s3c2440_fini(struct device_desc *dev)
{
	struct ts_s3c2440_io *io = (struct ts_s3c2440_io*)dev->data;
	free(dev->dev);
	free(io);
}

static void ts_s3c2440_reset(struct device_desc *dev)
{
//	struct touchscreen_device *ts_dev = (struct touchscreen_device*)dev->dev;
	struct ts_s3c2440_io *io = (struct ts_s3c2440_io*) dev->data;

	memset(io, 0, sizeof(struct ts_s3c2440_io));
}

static void ts_s3c2440_update(struct device_desc *dev)
{
//	struct device_interrupt *intr = &dev->intr;
//	struct touchscreen_device *ts_dev = (struct touchscreen_device*)dev->dev;
	struct ts_s3c2440_io *io = (struct ts_s3c2440_io*)dev->data;
	struct machine_config *mc = (struct machine_config*)dev->mach;

	if (mc->mach_pending_intr == NULL) return;

//	if ((!mc->mach_pending_intr(intr->interrupts[0]))) { /* if now has no ts interrupt, then query */
		if (Pen_buffer[6] == 1) { /* interrupt */

			*(io->ts_buffer + 0) = (1<<15);
			if (Pen_buffer[5] == 1) {	// state == down
				*(io->ts_buffer + 3) = ((319-Pen_buffer[1])*804/320)+87+(rand()%((rand()%13)+3));		// vertical
				*(io->ts_buffer + 4) = (Pen_buffer[0]*764/240)+148+(rand()%((rand()%13)+3));			// horizontal
			}
			else {
				*(io->ts_buffer + 3) = 0x3ff;
				*(io->ts_buffer + 4) = 0x3ff;
			}

//			ts_s3c2440_set_update_intr(dev); /* update interrupt, ts driver will clear it */
			Pen_buffer[6] = 0;
		}
//	}
}

static int ts_s3c2440_read_word(struct device_desc *dev, uint32_t addr, uint32_t *data)
{
//	struct touchscreen_device *ts_dev = (struct touchscreen_device*) dev->dev;
	struct ts_s3c2440_io *io = (struct ts_s3c2440_io*)dev->data;
	int offset = (addr & ~3) - dev->base;

	*data = io->ts_buffer[offset/4];

	return ADDR_HIT;
}

static int ts_s3c2440_write_word(struct device_desc *dev, uint32_t addr, uint32_t data)
{
	return ADDR_HIT;
}

static int ts_s3c2440_setup(struct device_desc *dev)
{
	struct ts_s3c2440_io *io;
//	struct device_interrupt *intr = &dev->intr;

	dev->fini = ts_s3c2440_fini;
	dev->reset = ts_s3c2440_reset;
	dev->update = ts_s3c2440_update;
	dev->read_word = ts_s3c2440_read_word;
	dev->write_word = ts_s3c2440_write_word;

	io = (struct ts_s3c2440_io*)malloc(sizeof(struct ts_s3c2440_io));
	if (io == NULL) return 1;

	dev->data = (void*)io;

	ts_s3c2440_reset(dev);

	/* see if we need to set default values. */
	set_device_default(dev, s3c2440_ts_def);

	return 0;
}

void touchscreen_s3c2440_init(struct device_module_set *mod_set)
{
	register_device_module("s3c2440", mod_set, &ts_s3c2440_setup);
}

