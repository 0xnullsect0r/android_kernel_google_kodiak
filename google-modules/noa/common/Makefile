# SPDX-License-Identifier: GPL-2.0
#
# Makefile for NOA driver
#

BUILD=$(findstring bazel, $(KERNEL_SRC))

ifeq ($(BUILD), bazel)
SRC_DIR=$(ROOT_DIR)/private/google-modules/noa/common
else
SRC_DIR=$(srctree)/drivers/soc/google/noa
endif

COMMON_DIR=common
COMMON_OBJ=$(COMMON_DIR)/core.o $(COMMON_DIR)/noatrace.o $(COMMON_DIR)/ring.o

obj-$(CONFIG_NOA_SUPPORT) += noa.o

# noa driver
noa-$(CONFIG_NOA_SUPPORT) += main.o debugfs.o

noa-$(CONFIG_NOA_SUPPORT) += $(COMMON_OBJ)

# noa wifi driver
noa-$(CONFIG_NOA_WLAN_SUPPORT) += wlan/noa_wlan.o wlan/debugfs.o \
	wlan/wlan_trace.o wlan/wlan_debug_controller/debugfs.o \
	wlan/wlan_debug_controller/wlan_debug_packet_sniffer_debugfs.o \
	wlan/wlan_debug_controller/wlan_debug_log_system_debugfs.o \
	wlan/wlan_debug_controller/wlan_debug_shared_info_fetcher_debugfs.o \
	wlan/wlan_debug_controller/wlan_debug_trace.o \
	wlan/wlan_rpc_service/noa_wlan_rpc.o \
	wlan/wlan_rpc_service/noa_wlan_cmd_dispatch.o \
	wlan/noa_wlan_cfg_space.o \
	wlan/noa_wlan_mapper.o \
	wlan/noa_wlan_nep_helper.o \
	wlan/noa_wlan_dynamic_switch.o

ifeq ($(CONFIG_NOA_FULLSOC_SUPPORT),m)
noa-$(CONFIG_NOA_WLAN_SUPPORT) += wlan/noa_wlan_hw.o
else
noa-$(CONFIG_NOA_WLAN_SUPPORT) += wlan/noa_wlan_hw_sim.o
endif

noa-$(CONFIG_NOA_WLAN_SUPPORT) += wlan/noa_wlan_buffer_management.o

##########################################################################
# Configuration for Samsung modem driver
##########################################################################
ifneq ($(CONFIG_NOA_MD_SAMSUNG_SUPPORT),)
# Add Samsung-specific object
noa-$(CONFIG_NOA_MD_SAMSUNG_SUPPORT) += md/samsung/noa_md_custom.o
noa-$(CONFIG_NOA_MD_SAMSUNG_SUPPORT) += md/samsung/debugfs.o
endif

##########################################################################
# Configuration for Mediatek modem driver && Full SoC mode
##########################################################################
ifneq ($(CONFIG_NOA_MD_MEDIATEK_SUPPORT),)
include $(SRC_DIR)/md/mediatek/Makefile
else ifneq ($(CONFIG_NOA_MD_MEDIATEK_FULLSOC_SUPPORT),)
include $(SRC_DIR)/md/mediatek/Makefile
endif

# NOA RPC support
ifeq ($(CONFIG_NOA_FULLSOC_SUPPORT),m)
include $(SRC_DIR)/Makefile.rpc
endif

# WiFi-Calling packet offload support (with netfilter egress hook).
NOA_VPN_EGRESS_OBJS-$(CONFIG_NOA_VPN_OFFLOAD_SUPPORT) := tether/noa_vpn_egress_handler.o

# tethering offload support
# TODO(b/367885986): Don't bind the NOA net module to the tethering_offload kconfig.
obj-$(CONFIG_NOA_TETHERING_OFFLOAD_SUPPORT) += noa_cont.o

noa_cont-$(CONFIG_NOA_TETHERING_OFFLOAD_SUPPORT) += tether/noa_netengine_manager.o
noa_cont-$(CONFIG_NOA_TETHERING_OFFLOAD_SUPPORT) += tether/noa_tethering_manager.o
noa_cont-$(CONFIG_NOA_TETHERING_OFFLOAD_SUPPORT) += tether/net/nep_table_manager.o
noa_cont-$(CONFIG_NOA_VPN_OFFLOAD_SUPPORT) += tether/noa_vpn_manager.o
noa_cont-$(CONFIG_NOA_VPN_OFFLOAD_SUPPORT) += tether/vpn/packed_xfrm.o
noa_cont-$(CONFIG_NOA_PPF_SUPPORT) += tether/noa_ppf_manager.o
noa_cont-$(CONFIG_NOA_WFC_OFFLOAD_SUPPORT) += $(NOA_VPN_EGRESS_OBJS-y)

noa_cont-$(CONFIG_NOA_FULLSOC_SUPPORT) += net/nep_cmd_rpc_service/noa_nep_cmd_rpc.o
noa_cont-$(CONFIG_NOA_FULLSOC_SUPPORT) += ../noah_lib/gen/pw_rpc_services/nep_cmd_service.pb.o
noa_cont-$(CONFIG_NOA_FULLSOC_SUPPORT) += net/nep_cmd_rpc_service/nep_cmd_rpc_client.o
noa_cont-$(CONFIG_NOA_FULLSOC_SUPPORT) += net/nep_cmd_rpc_service/nep_event_rpc_client.o

# simulator support
# NCP
obj-$(CONFIG_NOA_SIM_SUPPORT) += ncp_sim.o

NCP_DIR=sim/ncp
# NCP main process
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(NCP_DIR)/ncp.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(NCP_DIR)/ncp_wlan_middle_layer.o

# common
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(COMMON_OBJ)

WLAN_DIR:=$(NCP_DIR)/wlan

# NCP WiFi firmware Common
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/trace.o

ifneq ($(CONFIG_NOA_WIFI_FW_V1), )
# NCP WiFi Firmware
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/ncp_wlan_fw.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/buffer_manager.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/nep_utility.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/device.o
ncp_sim-$(CONFIG_NOA_WLAN_QCA_SUPPORT) += $(WLAN_DIR)/device_qca.o
ncp_sim-$(CONFIG_NOA_WLAN_BRCM_SUPPORT) += $(WLAN_DIR)/device_brcm.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DIR)/system_utility.o
else
# NCP WiFi New Firmware
WLAN_FW_DIR:=$(NCP_DIR)/wlan_build

WLAN_RING_DIR:=$(WLAN_FW_DIR)/modules/wlan_ring
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_RING_DIR)/wlan_ring.o
# ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_RING_DIR)/wlan_ring_adapter.o
# ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_RING_DIR)/wlan_nep_ring_adaptee.o
# ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_RING_DIR)/wlan_noa_hw_ring_adaptee.o

WLAN_NOA_RING_SVC_DIR:=$(WLAN_FW_DIR)/modules/noa_ring_svc
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_NOA_RING_SVC_DIR)/noa_ring_svc.o

WLAN_BM_MGMT_DIR:=$(WLAN_FW_DIR)/modules/wlan_buffer_management
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_BM_MGMT_DIR)/wlan_buffer_manager.o

WLAN_NEP_BUFFER_POOL_DIR:=$(WLAN_FW_DIR)/modules/wlan_nep_buffer_pool
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_NEP_BUFFER_POOL_DIR)/wlan_nep_buffer_pool.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_NEP_BUFFER_POOL_DIR)/wlan_nep_tx_buffer_pool.o

WLAN_STA_TBL_DIR:=$(WLAN_FW_DIR)/modules/sta_table
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_STA_TBL_DIR)/sta_table.o

WLAN_FLOW_ID_TABLE_DIR:=$(WLAN_FW_DIR)/modules/flow_id_table
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_FLOW_ID_TABLE_DIR)/flow_id_table.o

WLAN_RING_MGR_DIR:=$(WLAN_FW_DIR)/modules/wlan_ring_manager
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_RING_MGR_DIR)/wlan_ring_manager.o

WLAN_DP_DIR:=$(WLAN_FW_DIR)/core/dp
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DP_DIR)/wlan_dp.o

WLAN_PM_DIR:=$(WLAN_FW_DIR)/core/wlan_pm
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_PM_DIR)/wlan_pm.o

WLAN_SVC_DIR:=$(WLAN_FW_DIR)/core/wlan_svc
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SVC_DIR)/wlan_service.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SVC_DIR)/wlan_service_fsm.o

WLAN_SHARED_MEM_DIR:=$(WLAN_FW_DIR)/core/wlan_shared_mem
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SHARED_MEM_DIR)/memory_map_helper.o

WLAN_SYSIF_DIR:=$(WLAN_FW_DIR)/sys_if
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/interrupt/sys_if_interrupt_driver_mode.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/mailbox/sys_if_mailbox_driver_mode.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/io/sys_if_io_driver_mode.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/memory/sys_if_memory_driver_mode.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/noa_net/noa_net_linux.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_SYSIF_DIR)/workqueue/sys_if_workqueue_driver_mode.o

WLAN_WDEV_IF_DIR:=$(WLAN_FW_DIR)/wdev_if
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_WDEV_IF_DIR)/wdev_if.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_WDEV_IF_DIR)/hw_arch/brcm/wdev_if_hw_arch_brcm.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_WDEV_IF_DIR)/chip/brcm4390/wdev_if_chip_brcm_4390.o

WLAN_DEBUG_CTRL_DIR:=$(WLAN_FW_DIR)/wlan_debug_controller
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DEBUG_CTRL_DIR)/wlan_packet_sniffer/wlan_debug_packet_sniffer_controller.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DEBUG_CTRL_DIR)/wlan_packet_sniffer/wlan_debug_packet_sniffer.o
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_DEBUG_CTRL_DIR)/wlan_log_sys/wlan_debug_log_sys_controller.o

WLAN_EXT_SVC_DIR:=$(WLAN_FW_DIR)/ext_svc
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_EXT_SVC_DIR)/ext_svc.o

WLAN_LOG_DIR:=$(WLAN_FW_DIR)/wlan_log
ncp_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(WLAN_LOG_DIR)/wlan_log.o
endif

# NCP Samsung Modem Firmware
ifneq ($(CONFIG_NOA_MD_SAMSUNG_SUPPORT),)
ncp_sim-$(CONFIG_NOA_MD_SAMSUNG_SUPPORT) += $(NCP_DIR)/md/samsung/ncp_md_fw.o
endif

# NCP Mediatek modem simulator
ifneq ($(CONFIG_NOA_MD_MEDIATEK_SUPPORT),)
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/mediatek/ncp_md.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/mediatek/ncp_md_tx_data.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/mediatek/ncp_md_rx_data.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/modules/modem_fw_ring.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/modules/modem_interface.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/modules/ncp_md_irq.o
ncp_sim-$(CONFIG_NOA_MD_MEDIATEK_SUPPORT) += $(NCP_DIR)/md/modules/noa_md_apc2ncp_ring.o
endif

#NEP
obj-$(CONFIG_NOA_SIM_SUPPORT) += nep_sim.o

NEP_DIR=sim/nep
# NEP simulator
nep_sim-$(CONFIG_NOA_SIM_SUPPORT) += \
	$(NEP_DIR)/nep.o \
	$(NEP_DIR)/netengine.o \
	$(NEP_DIR)/dma_processor.o \
	$(NEP_DIR)/dma_simulator.o

# Ring service
nep_sim-$(CONFIG_NOA_SIM_SUPPORT) += \
	$(NEP_DIR)/port_init.o \
	$(NEP_DIR)/ring_service/ring_buffer_pool.o \
	$(NEP_DIR)/ring_manager_instance.o \
	$(NEP_DIR)/ring_manager_instance_singleton.o \
	$(NEP_DIR)/wlan_ring_manager_instance.o \
	$(NEP_DIR)/modem_ring_manager_instance.o \
	$(NEP_DIR)/ring_controller.o \
	$(NEP_DIR)/ring_descriptor.o \
	$(NEP_DIR)/network_pipeline_framework/nested_ring_task.o \
	$(NEP_DIR)/network_pipeline_framework/nested_ring_stage.o \
	$(NEP_DIR)/network_pipeline_framework/stage.o \
	$(NEP_DIR)/network_pipeline_framework/packet_table.o \
	$(NEP_DIR)/network_pipeline_framework/packet_initiator.o \
	$(NEP_DIR)/network_pipeline_framework/task_scheduler.o \
	$(NEP_DIR)/network_pipeline_framework/task_scheduler_bitmap.o \
	$(NEP_DIR)/network_pipeline_framework/engine.o \
	$(NEP_DIR)/network_pipeline_module/buffer_pool_engine.o \
	$(NEP_DIR)/network_pipeline_module/dma_engine.o \
	$(NEP_DIR)/network_pipeline_service/service.o \
	$(NEP_DIR)/network_pipeline_service/task_manager.o \
	$(NEP_DIR)/network_pipeline_service/nested_ring_service.o \
	$(NEP_DIR)/network_pipeline_service/wlan_data_path.o \
	$(NEP_DIR)/network_pipeline_service/release_option.o \
	$(NEP_DIR)/network_pipeline_service/ring_utils.o \
	$(NEP_DIR)/ring_pipeline_service/cached_buffer_pool.o \
	$(NEP_DIR)/ring_pipeline_service/ring_data_cache.o \
	$(NEP_DIR)/ring_pipeline_service/ring_data_route.o \
	$(NEP_DIR)/ring_pipeline_service/ring_isr_handler.o \
	$(NEP_DIR)/ring_pipeline_service/ring_doorbell_notifier.o \
	$(NEP_DIR)/ring_pipeline_service/packet_header_fetch.o \
	$(NEP_DIR)/ring_pipeline_service/receiver.o \
	$(NEP_DIR)/ring_pipeline_service/sender.o \
	$(NEP_DIR)/ring_pipeline_service/feedthrough.o \
	$(NEP_DIR)/ring_pipeline_service/obsolete_packet_handler.o

ifneq ($(CONFIG_NOA_MD_MEDIATEK_T900_SUPPORT),)
	nep_sim-$(CONFIG_NOA_MD_MEDIATEK_T900_SUPPORT) += $(NEP_DIR)/network_pipeline_service/alcedo_modem_data_path.o
else
	nep_sim-$(CONFIG_NOA_MD_SAMSUNG_SUPPORT) += $(NEP_DIR)/network_pipeline_service/lassen_modem_data_path.o
endif


# Tethering
nep_sim-$(CONFIG_NOA_SIM_SUPPORT) += \
	$(NEP_DIR)/nep_map_table.o \
	$(NEP_DIR)/nep_tables.o \
	$(NEP_DIR)/offload.o \
	$(NEP_DIR)/netengine_utils.o \
	$(NEP_DIR)/tethering_pipeline_service.o \
	$(NEP_DIR)/vpn_pipeline_service.o \
	$(NEP_DIR)/checksum.o \
	$(NEP_DIR)/util/ring_util.o \
	$(NEP_DIR)/util/memory_pool.o \
	$(NEP_DIR)/util/utilization_monitor.o

# Pixel Packet Filter
nep_sim-$(CONFIG_NOA_PPF_SUPPORT) += \
	$(NEP_DIR)/apf_interpreter.o \
	$(NEP_DIR)/nep_apf.o \
	$(NEP_DIR)/ppf_pipeline_service.o \
	$(NEP_DIR)/ra_packet.o \
	$(NEP_DIR)/ra_timers.o \
	$(NEP_DIR)/ra_proxy.o

# VPN offload in NOA simulation
nep_sim-$(NOA_SIM_VPN_OFFLOAD_SUPPORT) += \
	$(NEP_DIR)/noa_ipsec.o \
	$(NEP_DIR)/noa_ipsec_crypto.o

# Common
nep_sim-$(CONFIG_NOA_SIM_SUPPORT) += $(COMMON_OBJ)

# Ring Manager
obj-$(CONFIG_NOA_RING_MANAGER_SUPPORT) += noa_ring_manager.o
noa_ring_manager-$(CONFIG_NOA_RING_MANAGER_SUPPORT) += ring_service/ring_manager_main.o
noa_ring_manager-$(CONFIG_NOA_RING_MANAGER_SUPPORT) += \
		$(NEP_DIR)/ring_manager.o \
		$(NEP_DIR)/ring_shared_info_instance.o \
noa_ring_manager-$(CONFIG_NOA_FULLSOC_SUPPORT) += \
		ring_service/ring_service_rpc_client.o \
		../noah_lib/gen/pw_rpc_services/ring_event_service.pb.o
noa_ring_manager-$(CONFIG_NOA_RING_MANAGER_SUPPORT) += $(COMMON_OBJ)

# Net Perf Driver
obj-$(CONFIG_NOA_TEST_HARNESS) += noa_test_harness.o
noa_test_harness-$(CONFIG_NOA_TEST_HARNESS) += test_harness/noa_test_harness.o \
					 test_harness/harness_buffer_management.o \
					 test_harness/harness_doorbell.o \
					 test_harness/harness_memory_mapper.o \
					 test_harness/harness_ring.o \
					 test_harness/harness_wlan_device.o \
					 test_harness/harness_modem_device.o

noa_test_harness-$(CONFIG_NOA_TEST_HARNESS) += $(COMMON_OBJ)

NOAH_LIB_PATH := $(ROOT_DIR)/private/google-modules/noa/noah_lib
ccflags-y += -I$(NOAH_LIB_PATH)/pw_rpc_c/public
ccflags-y += -I$(NOAH_LIB_PATH)/gen/pw_rpc_services
ccflags-y += -I$(NOAH_LIB_PATH)/pw_rpc_service_client/public

NANOPB_PATH := $(ROOT_DIR)/private/google-modules/noa/common/nanopb
ccflags-y += -DPB_SYSTEM_HEADER=\"common/pb_linux.h\"
ccflags-y += -I$(NANOPB_PATH)

DPA_PATH := $(ROOT_DIR)/private/google-modules/dpa
ccflags-y += -I$(DPA_PATH)/include

subdir-ccflags-y += \
		-g \
		-I$(SRC_DIR) \
		-I$(SRC_DIR)/sim \
		-I$(SRC_DIR)/common \
		-I$(SRC_DIR)/tether \
		-I$(SRC_DIR)/tether/vpn/public \
		-I$(SRC_DIR)/sim/ncp \
		-I$(SRC_DIR)/sim/nep \
		-I$(SRC_DIR)/sim/nep/ring_service \
		-I$(SRC_DIR)/ring_service \
		-I$(SRC_DIR)/sim/ncp/md \
		-I$(SRC_DIR)/sim/ncp/md/modules \
		-I$(SRC_DIR)/sim/ncp/wlan_build \

# GS
GS_DIR = private/google-modules/soc/gs
ifneq ($(wildcard $(ROOT_DIR)/$(GS_DIR)),)
subdir-ccflags-y += -I $(ROOT_DIR)/$(GS_DIR)/include
endif
ifneq ($(wildcard $(COMMON_OUT_DIR)/$(GS_DIR)/Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(GS_DIR)/Module.symvers
endif
# RDO
RDO_DIR = private/google-modules/soc/rdo
ifneq ($(wildcard $(ROOT_DIR)/$(RDO_DIR)),)
subdir-ccflags-y += -I $(ROOT_DIR)/$(RDO_DIR)/include
endif
ifneq ($(wildcard $(COMMON_OUT_DIR)/$(RDO_DIR)/Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(RDO_DIR)/Module.symvers
endif
ifneq ($(wildcard $(COMMON_OUT_DIR)/$(RDO_DIR)/pci/google-pcie-v1_Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(RDO_DIR)/pci/google-pcie-v1_Module.symvers
endif
ifneq ($(wildcard $(COMMON_OUT_DIR)/$(RDO_DIR)/pci/google-pcie-v2_Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(RDO_DIR)/pci/google-pcie-v2_Module.symvers
endif

DPA_DIR = private/google-modules/dpa
ifneq ($(wildcard $(ROOT_DIR)/$(DPA_DIR)),)
subdir-ccflags-y += -I $(ROOT_DIR)/$(DPA_DIR)/include
endif
ifneq ($(wildcard  $(COMMON_OUT_DIR)/$(DPA_DIR)/google-dpa_Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(DPA_DIR)/google-dpa_Module.symvers
endif
ifneq ($(wildcard  $(COMMON_OUT_DIR)/$(DPA_DIR)/google-dpa.pixel_Module.symvers),)
EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/$(DPA_DIR)/google-dpa.pixel_Module.symvers
endif

modules modules_install clean compile_commands.json:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) W=1 SRC_DIR=$(SRC_DIR) BUILD=$(BUILD) \
	$(KBUILD_OPTIONS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
