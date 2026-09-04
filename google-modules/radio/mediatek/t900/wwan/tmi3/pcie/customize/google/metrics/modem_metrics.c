// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Google LLC.
 */

#include "modem_metrics.h"

#include <linux/pcie_google_if.h>

#include "metrics_collection.h"
#include "mtk_dev.h"
#include "mtk_fsm.h"
#include "radio-utils.h"

#define MODEM_PCIE_PORT_NUM (0)

static bool is_device_ready = false;

static int __maybe_unused get_pcie_channel_num(struct mtk_md_dev *mdev)
{
	(void)mdev;

	return MODEM_PCIE_PORT_NUM;
}

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE)
static int mcf_pull_pcie_link_state(struct mcf_pcie_link_state_info *data, void *priv)
{
	struct mtk_md_dev *mdev = priv;
	int channel_num = get_pcie_channel_num(mdev);
	int ret = google_pcie_link_state(channel_num);

	if (ret < 0)
		return ret;

	data->link_state = (u32)ret;
	return 0;
}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN)
static int mcf_pull_pcie_link_updown(struct mcf_pcie_link_updown_info *data, void *priv)
{
	struct google_pcie_power_stats link_up;
	struct google_pcie_power_stats link_down;
	struct mtk_md_dev *mdev = priv;
	int channel_num = get_pcie_channel_num(mdev);

	int ret = google_pcie_get_power_stats(channel_num, &link_up, &link_down);

	if (ret)
		return ret;

	data->link_up.count = link_up.count;
	data->link_up.duration_ms = link_up.duration;
	data->link_up.last_entry_ms = link_up.last_entry_ms;

	data->link_down.count = link_down.count;
	data->link_down.duration_ms = link_down.duration;
	data->link_down.last_entry_ms = link_down.last_entry_ms;

	return 0;
}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION)
static int mcf_pull_pcie_link_duration(struct mcf_pcie_link_duration_info *data, void *priv)
{
	struct google_pcie_link_duration_stats link_duration;
	int max_link_speed = min(GPCIE_NUM_LINK_SPEEDS, MCF_MAX_PCIE_LINK_SPEED);
	struct mtk_md_dev *mdev = priv;
	int channel_num = get_pcie_channel_num(mdev);

	int ret = google_pcie_get_link_duration(channel_num, &link_duration);
	if (ret)
		return ret;

	data->last_link_speed = link_duration.last_link_speed;
	for (int i = 0; i < max_link_speed; ++i) {
		data->speed[i].count = link_duration.speed[i].count;
		data->speed[i].duration_ms = link_duration.speed[i].duration;
		data->speed[i].last_entry_ms = link_duration.speed[i].last_entry_ts;
	}

	return 0;
}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS)
static int mcf_pull_pcie_link_stats(struct mcf_pcie_link_stats_info *data, void *priv)
{
	struct google_pcie_link_stats link_stats;
	struct mtk_md_dev *mdev = priv;
	int channel_num = get_pcie_channel_num(mdev);

	int ret = google_pcie_get_link_stats(channel_num, &link_stats);
	if (ret)
		return ret;

	data->link_up_failure_count = link_stats.link_up_failure_count;
	data->link_recovery_failure_count = link_stats.link_recovery_failure_count;
	data->link_down_irq_count = link_stats.link_down_irq_count;
	data->cmpl_timeout_irq_count = link_stats.complete_timeout_irq_count;
	data->link_up_time_avg = link_stats.link_up_time_avg;

	return 0;
}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS

void modem_metrics_fsm_state_handler(struct radio_google *goog, struct mtk_fsm_param *param)
{
	switch (param->to) {
	case FSM_STATE_ON:
		is_device_ready = false;
		break;

	case FSM_STATE_POSTDUMP:
		mcf_notify_modem_boot_end(MODEM_BOOT_TYPE_MASK_DUMP);
		break;

	case FSM_STATE_READY:
		if (is_device_ready)
			break;

		is_device_ready = true;
		mcf_notify_modem_boot_end(MODEM_BOOT_TYPE_MASK_NORMAL |
					  MODEM_BOOT_TYPE_MASK_WARM_RESET);
		break;

	default:
		break;
	}
}

int modem_metrics_init(struct radio_google *goog)
{
	struct mtk_md_dev __maybe_unused *mdev = goog->mdev;
	int __maybe_unused ret = 0;

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE)
	ret = mcf_register_pcie_link_state(mcf_pull_pcie_link_state, mdev);
	if (ret) {
		LOG_ERR("Failed to register PCIe link state to mcf, ret = %d\n", ret);
		return ret;
	}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN)
	ret = mcf_register_pcie_link_updown(mcf_pull_pcie_link_updown, mdev);
	if (ret) {
		LOG_ERR("Failed to register PCIe link updown to mcf, ret = %d\n", ret);
		goto err_unregister_link_state;
	}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION)
	ret = mcf_register_pcie_link_duration(mcf_pull_pcie_link_duration, mdev);
	if (ret) {
		LOG_ERR("Failed to register PCIe link duration to mcf, ret = %d\n", ret);
		goto err_unregister_link_updown;
	}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS)
	ret = mcf_register_pcie_link_stats(mcf_pull_pcie_link_stats, mdev);
	if (ret) {
		LOG_ERR("Failed to register PCIe link stats to mcf, ret = %d\n", ret);
		goto err_unregister_link_duration;
	}
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS

	return ret;

err_unregister_link_duration:
	__maybe_unused
#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION)
		mcf_unregister_pcie_link_duration(mcf_pull_pcie_link_duration, mdev);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION

err_unregister_link_updown:
	__maybe_unused
#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN)
		mcf_unregister_pcie_link_updown(mcf_pull_pcie_link_updown, mdev);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN

err_unregister_link_state:
	__maybe_unused
#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE)
		mcf_unregister_pcie_link_state(mcf_pull_pcie_link_state, mdev);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE

	return ret;
}

void modem_metrics_exit(struct radio_google *goog)
{
	struct mtk_md_dev __maybe_unused *mdev = goog->mdev;
	int __maybe_unused ret;

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS)
	ret = mcf_unregister_pcie_link_stats(mcf_pull_pcie_link_stats, mdev);
	if (ret)
		LOG_ERR("Failed to unregister PCIe link stats from mcf, ret = %d\n", ret);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATS

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION)
	ret = mcf_unregister_pcie_link_duration(mcf_pull_pcie_link_duration, mdev);
	if (ret)
		LOG_ERR("Failed to unregister PCIe link duration from mcf, ret = %d\n", ret);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_DURATION

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN)
	ret = mcf_unregister_pcie_link_updown(mcf_pull_pcie_link_updown, mdev);
	if (ret)
		LOG_ERR("Failed to unregister PCIe link updown from mcf, ret = %d\n", ret);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_UPDOWN

#if IS_ENABLED(CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE)
	ret = mcf_unregister_pcie_link_state(mcf_pull_pcie_link_state, mdev);
	if (ret)
		LOG_ERR("Failed to unregister PCIe link state from mcf, ret = %d\n", ret);
#endif // CONFIG_GOOGLE_METRIC_PCIE_LINK_STATE
}
