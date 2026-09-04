/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Google LLC.
 *
 */

#ifndef FWMT_IPC_H
#define FWMT_IPC_H

#include "fwmt_driver.h"
#include "fwmt_service.h"

struct mba_buffer_info {
	char *mba_buffer;
	dma_addr_t mba_buffer_pa;
	size_t mba_buffer_size;
};

struct cdev_state {
	struct fwmt_base *base;
	struct mba_buffer_info mbi;
	enum fwmt_mba_op_type op_type;

	uint32_t total_size;
};

struct cdev_state *get_cdev_state(struct fwmt_base *base,
				  enum fwmt_mba_op_type op_type);

void put_cdev_state(struct cdev_state *cdev_state);

int64_t refill_mba_buffer(struct cdev_state *cdev_state, size_t len,
			  loff_t offset, uint32_t *p_size);

#endif /* FWMT_IPC_H */
