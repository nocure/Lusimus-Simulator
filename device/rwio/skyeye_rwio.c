/*
 * skyeye_rwio.c
 * 		Real world file I/O, for reading from / writing to real files.
 *
 *  Created on: 2014/1/13
 *      Author: ww
 */

#include <stdlib.h>
#include "skyeye_device.h"
#include "skyeye_options.h"
#include "skyeye.h"
#include "skyeye_rwio.h"


static void rwio_init (struct device_module_set *mod_set)
{
	rwio_type1_init(mod_set);
}


static int rwio_setup(struct device_desc *dev, void *option)
{
	struct rwio_device *rwio_dev;
//	struct rwio_option *rwio_opt = (struct rwio_option*)option;
	int ret = 0;

	rwio_dev = (struct rwio_device*)malloc(sizeof(struct rwio_device));
	if (rwio_dev == NULL) return 1;

	memset(rwio_dev, 0, sizeof(struct rwio_device));

	/* TODO */

	dev->dev = (void*)rwio_dev;
	return ret;
}


static struct device_module_set rwio_mod_set = {
	.name = "rwio",
	.count = 0,
	.count_max = 0,
	.init = rwio_init,
	.initialized = 0,
	.setup_module = rwio_setup,
};

/*
 * rwio_register() : Used by global device initialize function.
 */
void rwio_register(void)
{
	if (register_device_module_set(&rwio_mod_set)) {
		SKYEYE_ERR("\"%s\" module set register error!\n", rwio_mod_set.name);
	}
}

