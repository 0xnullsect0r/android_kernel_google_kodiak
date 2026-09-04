#ifndef NOA_ARCH_NOA_ARCH_H
#define NOA_ARCH_NOA_ARCH_H

#include "sys_if/common.h"
#include "sys_if/types/types.h"

enum {
	kVendorRxBuffer = 0,
	kNoaTxBuffer,
};

/// @brief General NOA descriptor.
typedef struct noa_desc NoaDesc;

/// @brief General NOA buffer pool descriptor.
typedef struct noa_buffer_pool_desc NoaBufferPoolDesc;

/// @brief Net engine NOA TX descriptor.
typedef struct network_ext_txd NoaNetworkTxD;

/// @brief Net engine NOA RX descriptor.
typedef struct network_ext_rxd NoaNetworkRxD;

/// @brief Broadcom-specific NOA TX descriptor.
typedef struct NoaBrcmTxD {
	/// @brief Flags for the descriptor.
	uint8_t flags;
	/// @brief Extended flags for the descriptor.
	uint8_t ext_flags;
	/// @brief Interface index.
	uint8_t ifidx;
	/// @brief Current phase of the device ring.
	uint8_t current_phase;
	/// @brief Flow ID.
	uint8_t ring_id;
	/// @brief Extended tag.
	uint8_t ext_tag;
	/// @brief Packet checksum type.
	uint8_t pkt_csum_type;
	/// @brief Length of the Layer 3 header.
	uint8_t l3_hdr_len;
	/// @brief Length of the Layer 4 header.
	uint8_t l4_hdr_len;
	/// @brief Reserved for alignment.
	uint8_t reserved;
	/// @brief Ethernet type.
	uint16_t ethertype;
} __attribute__((packed, aligned(4))) NoaBrcmTxD;

typedef union NoaWlanExtendTxD {
	NoaBrcmTxD brcm_txd;
} NoaWlanExtendTxD;

typedef struct NoaWlanBufferReplnDesc {
	uint8_t to_dev;
	uint8_t rsv[3];
	uint16_t tkid;
	uint16_t len;
	uint64_t pa;
	uint64_t dpa_va;
} __attribute__((packed, aligned(4))) NoaWlanBufferReplnDesc;

#endif /* NOA_ARCH_NOA_ARCH_H */
