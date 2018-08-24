/*
 * dev_sefsio.c
 *
 *  Created on: 2015/10/29
 *      Author: user
 */

#include <stdint.h>
#include <stdlib.h>
#include <sys\stat.h>
#include "skyeye_device.h"
#include "skyeye_sefsio.h"

#define SEFSIO_FUNC		0x0
#define SEFSIO_PARAM1	0x1
#define SEFSIO_PARAM2	0x2
#define SEFSIO_PARAM3	0x3
#define SEFSIO_MAX		0x4

#define FUNC_fopen		0
#define FUNC_fclose		1
#define FUNC_fseek		2
#define FUNC_ftell		3
//#define FUNC_stat		4
//#define FUNC_truncate	5
#define FUNC_fputs		6
#define FUNC_feof		7
#define FUNC_fputc		8
#define FUNC_fgetc		9
#define FUNC_fgets		10
#define FUNC_fread		11
#define FUNC_fwrite		12
#define FUNC_rename		13
#define FUNC_unlink		14
#define FUNC_fflush		15
#define FUNC_tmpfile	16
#define FUNC_ferror		17


struct sefsio_type1_io {
	uint32_t reg[4];
};


static struct device_default_value sefsio_type1_def[] = {
	/* name			base		size	interrupt array */
	{"type1",		0x67ffffe0,	sizeof(struct sefsio_type1_io),	{0, 0, 0, 0}},
	{NULL},
};


static void sefsio_type1_fini(struct device_desc *dev)
{
	struct sefsio_device *sefsio_dev=(struct sefsio_device*)dev->dev;
	struct sefsio_type1_io *io = (struct sefsio_type1_io*)dev->data;
	free(sefsio_dev->root);
	free(dev->dev);
	free(io);
}

static void sefsio_type1_reset(struct device_desc *dev)
{
//	struct sefsio_device *sefsio_dev=(struct sefsio_device*)dev->dev;
	struct sefsio_type1_io *io = (struct sefsio_type1_io*) dev->data;

	memset(io, 0, sizeof(struct sefsio_type1_io));
}


static int sefsio_type1_read_word(struct device_desc *dev, uint32_t addr, uint32_t *data)
{
	struct sefsio_type1_io *io = (struct sefsio_type1_io*)dev->data;
	int offset = ((addr & ~3) - dev->base)/4;

	if (offset>=SEFSIO_MAX) return ADDR_NOHIT;

	*data = io->reg[offset];

	return ADDR_HIT;
}


static void _build_path(char* root, char* target_file, char* path) {
char drive[_MAX_DRIVE];
char dir[_MAX_DIR];
char fname[_MAX_FNAME];
char ext[_MAX_EXT];

	_splitpath(target_file,drive,dir,fname,ext);
	_makepath(path,0,root,fname,ext);
}


extern unsigned char * get_dma_addr(unsigned long guest_addr);
static int sefsio_type1_write_word(struct device_desc *dev, uint32_t addr, uint32_t data)
{
uint32_t p1,p2,p3,ret;
struct sefsio_device *sefsio_dev=(struct sefsio_device*)dev->dev;
struct sefsio_type1_io *io = (struct sefsio_type1_io*)dev->data;
int offset = ((addr & ~3) - dev->base)/4;

char path[_MAX_PATH];
char path2[_MAX_PATH];

	if (offset>=SEFSIO_MAX) return ADDR_NOHIT;

	if (offset==SEFSIO_FUNC) {
		switch(data) {
		case FUNC_fopen:
				p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
				p2=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM2]);
				_build_path(sefsio_dev->root,(char*)p1,path);
				ret=(uint32_t)fopen((const char*)path,(const char*)p2);
			break;

		case FUNC_fclose:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			ret=(uint32_t)fclose((FILE*)p1);
			break;

		case FUNC_fseek:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			p3=(uint32_t)io->reg[SEFSIO_PARAM3];
			ret=(uint32_t)fseek((FILE*)p1,(off_t)p2,(int)p3);
			break;

		case FUNC_ftell:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			ret=(uint32_t)ftell((FILE*)p1);
			break;
#if 0
		case FUNC_stat:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM2]);
			ret=(uint32_t)stat((const char*)p1,(struct stat*)p2);
			break;
#endif
		case FUNC_fputs:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			ret=(uint32_t)fputs((const char*)p1,(FILE *)p2);
			break;

		case FUNC_feof:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			ret=(uint32_t)feof((FILE *)p1);
			break;

		case FUNC_fputc:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			ret=(uint32_t)fputc((int)p1,(FILE *)p2);
			break;

		case FUNC_fgets:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			p3=(uint32_t)io->reg[SEFSIO_PARAM3];
			ret=(uint32_t)fgets((char*)p1,(int)p2,(FILE *)p3);
			if (ret) ret=io->reg[SEFSIO_PARAM1];
			break;

		case FUNC_fread:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			p3=(uint32_t)io->reg[SEFSIO_PARAM3];
			ret=(uint32_t)fread((void*)p1,1,(size_t)p2,(FILE *)p3);
			break;

		case FUNC_fwrite:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)io->reg[SEFSIO_PARAM2];
			p3=(uint32_t)io->reg[SEFSIO_PARAM3];
			ret=(uint32_t)fwrite((void*)p1,1,(size_t)p2,(FILE *)p3);
			break;

		case FUNC_rename:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			p2=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM2]);
			_build_path(sefsio_dev->root,(char*)p1,path);
			_build_path(sefsio_dev->root,(char*)p2,path2);
			ret=(uint32_t)rename(path,path2);
			break;

		case FUNC_unlink:
			p1=(uint32_t)get_dma_addr((unsigned long)io->reg[SEFSIO_PARAM1]);
			_build_path(sefsio_dev->root,(char*)p1,path);
			ret=(uint32_t)unlink(path);
			break;

		case FUNC_fflush:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			ret=(uint32_t)fflush((FILE*)p1);
			break;

		case FUNC_tmpfile:
			ret=(uint32_t)tmpfile();
			break;

		case FUNC_ferror:
			p1=(uint32_t)io->reg[SEFSIO_PARAM1];
			ret=(uint32_t)ferror((FILE*)p1);
			break;

		default:
			ret=0;
		}

		io->reg[SEFSIO_FUNC]=ret;

	}
	else {
		io->reg[offset]=data;
	}

	return ADDR_HIT;
}


static int sefsio_type1_setup(struct device_desc *dev)
{
	struct sefsio_type1_io *io;
//	struct device_interrupt *intr = &dev->intr;

	dev->fini = sefsio_type1_fini;
	dev->reset = sefsio_type1_reset;
	dev->update = 0;
	dev->read_word = sefsio_type1_read_word;
	dev->write_word = sefsio_type1_write_word;

	io = (struct sefsio_type1_io*)malloc(sizeof(struct sefsio_type1_io));
	if (io == NULL) return 1;

	dev->data = (void*)io;

	sefsio_type1_reset(dev);

	/* see if we need to set default values. */
	set_device_default(dev, sefsio_type1_def);

	return 0;
}

void sefsio_type1_init(struct device_module_set *mod_set)
{
	register_device_module("1", mod_set, &sefsio_type1_setup);
}

