/*
 * skyeye_sefsio.h
 *
 *  Created on: 2015/10/29
 *      Author: user
 */

#ifndef SKYEYE_SEFSIO_H_
#define SKYEYE_SEFSIO_H_

struct sefsio_device {
	char* root;
};

void sefsio_type1_init(struct device_module_set *mod_set);

#endif /* SKYEYE_SEFSIO_H_ */
