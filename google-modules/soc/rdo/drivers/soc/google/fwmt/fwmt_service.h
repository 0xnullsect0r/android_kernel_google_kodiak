/* SPDX-License-Identifier: GPL-2.0-only OR MIT */
/*
 * Copyright 2024 Google LLC.
 *
 * Google firmware tracepoint protocol header.
 *
 * This header is copied from the Pixel firmware sources to the Linux kernel
 * sources, so it's written to be compiled under both Linux and the firmware,
 * and it's licensed under GPL or MIT licenses.
 */

#ifndef __FWMT_SERVICE_H
#define __FWMT_SERVICE_H

#ifdef __linux__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/*
 * SERVICE_ID: GDMC_MBA_SERVICE_ID_FWMT
 *
 * This message is used to retrieve metric info from GDMC SSWRP.
 *
 * The TYPE field in the header determines type of resource to be retrieved.
 *
 * Request:
 * AP must pass physical address and available size of the buffer where
 * requested resource should be stored.
 * NOTE: Buffer must be placed in the first gigabyte of DRAM
 * (SoC range: 0x80000000 - 0xBFFFFFFF)
 *
 * Word 0: Non-queue mode header and data
 *  bit [ 31 - 16       | 15-0   ]
 *      | common header | TYPE   |
 * Word 1: Bits 31-0 of SOC physical address of the buffer
 * Word 2: Bits 63-32 of SOC physical address of the buffer
 * Word 3: Buffer size
 *
 * Buffer size must be at least sizeof(struct gdmc_mba_fwmt_msg_request_buffer)
 * and the first 4 bytes shall contain offset of the requested data
 * in the given resource.
 * |        Buffer       |
 * | offset |    rest    |
 *
 * ====================================================
 * ===== TYPE: GDMC_MBA_FWMT_RETRIEVE_STRING ==========
 * ====================================================
 *
 * This operation is used to retrieve string table from SRAM.
 *
 * Response:
 * String table is placed in the buffer.
 *
 * Word 0: (unchanged)
 * Word 1: Number of bytes written in the buffer
 * Word 2: Total size of the string table
 * Word 3: (unchanged)
 *
 * ====================================================
 * ===== TYPE: GDMC_MBA_FWMT_RETRIEVE_METRIC ==========
 * ====================================================
 *
 * This operation is used to retrieve metric array from SRAM.
 *
 * Response:
 * Metrics are placed in the buffer.
 *
 * Word 0: (unchanged)
 * Word 1: Number of bytes written in the buffer
 * Word 2: Total size of the metric array
 * Word 3: (unchanged)
 */

/* Valid values for the TYPE field. */
enum fwmt_mba_op_type {
	GDMC_MBA_FWMT_RETRIEVE_STRING = 0,
	GDMC_MBA_FWMT_RETRIEVE_METRIC = 1,
};

/* MBA message structure for FWMT service */
struct fwmt_mba_msg {
	uint32_t header;
	union {
		struct {
			uint32_t pa_low;
			uint32_t pa_high;
			uint32_t buffer_capacity;
		} request;

		struct {
			uint32_t size;
			uint32_t total_size;
			uint32_t rsv;
		} response;
	} payload;
};

_Static_assert(sizeof(struct fwmt_mba_msg) == 4 * sizeof(uint32_t),
	       "fwmt_mba_msg size");

struct fwmt_msg_request_buffer {
	union {
		uint32_t resource_offset;
		uint8_t data[];
	};
};

_Static_assert(sizeof(struct fwmt_msg_request_buffer) == sizeof(uint32_t),
	       "fwmt_msg_request_buffer size");

#endif /* __FWMT_SERVICE_H */
