/*
 * dev_rwio.c
 *
 *  Created on: 2014/1/13
 *      Author: ww
 */
#include <stdint.h>
#include <stdlib.h>
#include "skyeye_device.h"

#define RWIOCTRL	0x0
#define RWIONAME	0x1
#define RWIOADDR	0x2
#define RWIOSIZE	0x3
#define RWIO_MAX	0x4


struct rwio_type1_io {
	uint32_t reg[4];
};


static struct device_default_value rwio_type1_def[] = {
	/* name			base		size	interrupt array */
	{"type1",		0x67fffff0,	sizeof(struct rwio_type1_io),	{0, 0, 0, 0}},
	{NULL},
};


static void rwio_type1_fini(struct device_desc *dev)
{
	struct rwio_type1_io *io = (struct rwio_type1_io*)dev->data;
	free(dev->dev);
	free(io);
}

static void rwio_type1_reset(struct device_desc *dev)
{
//	struct touchscreen_device *ts_dev = (struct touchscreen_device*)dev->dev;
	struct rwio_type1_io *io = (struct rwio_type1_io*) dev->data;

	memset(io, 0, sizeof(struct rwio_type1_io));
}


static int rwio_type1_read_word(struct device_desc *dev, uint32_t addr, uint32_t *data)
{
//	struct touchscreen_device *ts_dev = (struct touchscreen_device*) dev->dev;
	struct rwio_type1_io *io = (struct rwio_type1_io*)dev->data;
	int offset = ((addr & ~3) - dev->base)/4;
//	int ret = ADDR_HIT;

	if (offset>=RWIO_MAX) return ADDR_NOHIT;

	*data = io->reg[offset];

	return ADDR_HIT;
}


extern unsigned char * get_dma_addr(unsigned long guest_addr);
static int rwio_type1_write_word(struct device_desc *dev, uint32_t addr, uint32_t data)
{
char* vname;
char* vaddr;
uint32_t size,ret_size;
FILE *f;
struct rwio_type1_io *io = (struct rwio_type1_io*)dev->data;
int offset = ((addr & ~3) - dev->base)/4;

	if (offset>=RWIO_MAX) return ADDR_NOHIT;

	if (offset==RWIOCTRL) {
		size=io->reg[RWIOSIZE];
		vaddr=(char*)get_dma_addr((unsigned long)io->reg[RWIOADDR]);
		vname=(char*)get_dma_addr((unsigned long)io->reg[RWIONAME]);
		if (data==0) {	// read
			f=fopen(vname,"rb");
			ret_size=fread(vaddr,1,size,f);
			io->reg[RWIOSIZE]=ret_size;
		}
		else {	// wirte
			f=fopen(vname,"wb");
			ret_size=fwrite(vaddr,1,size,f);
			if (ret_size!=size) {
				fprintf(stderr,"result of a file operstion does not match requirement.");
			}
		}
		io->reg[RWIOSIZE]=ret_size;
		fclose(f);
	}
	else {
		io->reg[offset]=data;
	}

	return ADDR_HIT;
}

static int rwio_type1_setup(struct device_desc *dev)
{
	struct rwio_type1_io *io;
//	struct device_interrupt *intr = &dev->intr;

	dev->fini = rwio_type1_fini;
	dev->reset = rwio_type1_reset;
	dev->update = 0;
	dev->read_word = rwio_type1_read_word;
	dev->write_word = rwio_type1_write_word;

	io = (struct rwio_type1_io*)malloc(sizeof(struct rwio_type1_io));
	if (io == NULL) return 1;

	dev->data = (void*)io;

	rwio_type1_reset(dev);

	/* see if we need to set default values. */
	set_device_default(dev, rwio_type1_def);

	return 0;
}

void rwio_type1_init(struct device_module_set *mod_set)
{
	register_device_module("1", mod_set, &rwio_type1_setup);
}

