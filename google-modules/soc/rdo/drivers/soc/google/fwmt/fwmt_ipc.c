// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Google LLC.
 *
 */

#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <soc/google/goog_gdmc_service_ids.h>
#include <soc/google/goog-mba-gdmc-iface.h>
#include <soc/google/goog_mba_nq_xport.h>

#include "fwmt_driver.h"
#include "fwmt_ipc.h"
#include "fwmt_service.h"

struct cdev_state *get_cdev_state(struct fwmt_base *base,
				  enum fwmt_mba_op_type op_type)
{
	struct cdev_state *cdev_state;

	cdev_state = kmalloc(sizeof(struct cdev_state), GFP_KERNEL);
	if (!cdev_state)
		return ERR_PTR(-ENOMEM);

	cdev_state->base = base;

	cdev_state->mbi.mba_buffer = base->mba_buffer;
	cdev_state->mbi.mba_buffer_pa = base->mba_buffer_paddr;
	cdev_state->mbi.mba_buffer_size = base->mba_buffer_size;

	cdev_state->op_type = op_type;
	cdev_state->total_size = U32_MAX;

	return cdev_state;
}

void put_cdev_state(struct cdev_state *cdev_state)
{
	kfree(cdev_state);
}

static int64_t handle_mailbox_error(struct device *dev, int message_res,
				    uint32_t *header)
{
	if (goog_mba_nq_xport_get_error(header)) {
		/* GDMC firmware error code is int16_t, alignment is required */
		return (int16_t)goog_mba_nq_xport_get_data(header);
	}

	return message_res;
}

int64_t refill_mba_buffer(struct cdev_state *cdev_state, size_t len,
			  loff_t offset, uint32_t *p_size)
{
	struct fwmt_base *base = cdev_state->base;
	struct mba_buffer_info *mbi = &cdev_state->mbi;
	struct fwmt_msg_request_buffer *mba_buffer =
		(struct fwmt_msg_request_buffer *)mbi->mba_buffer;
	uint32_t bytes_to_copy;
	struct fwmt_mba_msg msg;
	int message_res;
	int64_t ret;

	bytes_to_copy = mbi->mba_buffer_size;
	if (bytes_to_copy > len)
		bytes_to_copy = len;

	msg.header = goog_mba_nq_xport_create_hdr(GDMC_MBA_SERVICE_ID_FWMT,
						  cdev_state->op_type);
	msg.payload.request.pa_low = (uint32_t)mbi->mba_buffer_pa;
	msg.payload.request.pa_high = (uint32_t)(mbi->mba_buffer_pa >> 32);
	msg.payload.request.buffer_capacity = bytes_to_copy;
	mba_buffer->resource_offset = offset;

	/* Write memory barrier to sync previous write with MBA recipient */
	wmb();

	message_res = gdmc_send_message(base->gdmc_iface, &msg);

	ret = handle_mailbox_error(base->dev, message_res, &msg.header);
	if (ret) {
		dev_err(base->dev, "FWMT Mailbox request failed: %lld.\n", ret);
		return ret;
	}

	if (msg.payload.response.size > bytes_to_copy) {
		dev_err(base->dev, "Malformed MBA response\n");
		return -EFAULT;
	}

	/* Read memory barrier to sync buffer contents with the kernel */
	rmb();

	cdev_state->total_size = msg.payload.response.total_size;
	*p_size = msg.payload.response.size;

	return 0;
}
