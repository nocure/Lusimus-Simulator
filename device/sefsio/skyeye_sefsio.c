/*
 * skyeye_sefsio.c
 *
 *  Created on: 2015/10/29
 *      Author: user
 */

#include <stdlib.h>
#include "skyeye_device.h"
#include "skyeye_options.h"
#include "skyeye.h"
#include "skyeye_sefsio.h"

static void sefsio_init (struct device_module_set *mod_set)
{
	sefsio_type1_init(mod_set);
}


static int sefsio_setup(struct device_desc *dev, void *option)
{
	struct sefsio_device *sefsio_dev;
	struct sefsio_option *sefsio_opt = (struct sefsio_option*)option;
	int ret = 0;

	sefsio_dev = (struct sefsio_device*)malloc(sizeof(struct sefsio_device));
	if (sefsio_dev == NULL) return 1;

	memset(sefsio_dev, 0, sizeof(struct sefsio_device));

	sefsio_dev->root=sefsio_opt->root;

	dev->dev = (void*)sefsio_dev;
	return ret;
}


static struct device_module_set sefsio_mod_set = {
	.name = "sefsio",
	.count = 0,
	.count_max = 0,
	.init = sefsio_init,
	.initialized = 0,
	.setup_module = sefsio_setup,
};

/*
 * sefsio_register() : Used by global device initialize function.
 */
void sefsio_register(void)
{
	if (register_device_module_set(&sefsio_mod_set)) {
		SKYEYE_ERR("\"%s\" module set register error!\n", sefsio_mod_set.name);
	}
}

