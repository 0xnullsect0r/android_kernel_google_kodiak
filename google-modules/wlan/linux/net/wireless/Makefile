# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_CFG80211) += cfg80211.o
obj-$(CONFIG_LIB80211) += lib80211.o
obj-$(CONFIG_LIB80211_CRYPT_WEP) += lib80211_crypt_wep.o
obj-$(CONFIG_LIB80211_CRYPT_CCMP) += lib80211_crypt_ccmp.o
obj-$(CONFIG_LIB80211_CRYPT_TKIP) += lib80211_crypt_tkip.o
obj-y += tests/

obj-$(CONFIG_WEXT_CORE) += wext-core.o
obj-$(CONFIG_WEXT_PROC) += wext-proc.o
obj-$(CONFIG_WEXT_SPY) += wext-spy.o
obj-$(CONFIG_WEXT_PRIV) += wext-priv.o

cfg80211-y += core.o sysfs.o radiotap.o util.o reg.o scan.o nl80211.o
cfg80211-y += mlme.o ibss.o sme.o chan.o ethtool.o mesh.o ap.o trace.o ocb.o
cfg80211-y += pmsr.o
cfg80211-$(CONFIG_OF) += of.o
cfg80211-$(CONFIG_CFG80211_DEBUGFS) += debugfs.o
cfg80211-$(CONFIG_CFG80211_WEXT) += wext-compat.o wext-sme.o

CFLAGS_trace.o := -I$(src)

cfg80211-$(CONFIG_CFG80211_USE_KERNEL_REGDB_KEYS) += shipped-certs.o
ifneq ($(CONFIG_CFG80211_EXTRA_REGDB_KEYDIR),)
cfg80211-y += extra-certs.o
endif

clean-files += extra-certs.c
