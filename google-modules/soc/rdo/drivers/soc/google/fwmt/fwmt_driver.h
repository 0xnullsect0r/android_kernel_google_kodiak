/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Google LLC.
 *
 */

#ifndef FWMT_DRIVER_H
#define FWMT_DRIVER_H

#include <linux/cdev.h>
#include <linux/mutex.h>

struct device;
struct gdmc_iface;
struct fwmt_base;

struct fwmt_cdev_instance {
	struct cdev cdev;
	dev_t devt;
	struct fwmt_base *base;
};

struct fwmt_base {
	struct gdmc_iface *gdmc_iface;
	struct device *dev;
	struct class *class;
	dev_t devt;

	struct fwmt_cdev_instance cdev_metrics;
	struct fwmt_cdev_instance cdev_strings;

	void *mba_buffer;
	phys_addr_t mba_buffer_paddr;
	size_t mba_buffer_size;
	struct mutex buffer_lock;
};

#endif /* FWMT_DRIVER_H */
