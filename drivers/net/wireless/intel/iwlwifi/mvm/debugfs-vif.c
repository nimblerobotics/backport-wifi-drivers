// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (C) 2012-2014, 2018-2023 Intel Corporation
 * Copyright (C) 2013-2015 Intel Mobile Communications GmbH
 * Copyright (C) 2016-2017 Intel Deutschland GmbH
 */
#include "mvm.h"
#include "debugfs.h"

static void iwl_dbgfs_update_pm(struct iwl_mvm *mvm,
				 struct ieee80211_vif *vif,
				 enum iwl_dbgfs_pm_mask param, int val)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_dbgfs_pm *dbgfs_pm = &mvmvif->dbgfs_pm;

	dbgfs_pm->mask |= param;

	switch (param) {
	case MVM_DEBUGFS_PM_KEEP_ALIVE: {
		int dtimper = vif->bss_conf.dtim_period ?: 1;
		int dtimper_msec = dtimper * vif->bss_conf.beacon_int;

		IWL_DEBUG_POWER(mvm, "debugfs: set keep_alive= %d sec\n", val);
		if (val * MSEC_PER_SEC < 3 * dtimper_msec)
			IWL_WARN(mvm,
				 "debugfs: keep alive period (%ld msec) is less than minimum required (%d msec)\n",
				 val * MSEC_PER_SEC, 3 * dtimper_msec);
		dbgfs_pm->keep_alive_seconds = val;
		break;
	}
	case MVM_DEBUGFS_PM_SKIP_OVER_DTIM:
		IWL_DEBUG_POWER(mvm, "skip_over_dtim %s\n",
				val ? "enabled" : "disabled");
		dbgfs_pm->skip_over_dtim = val;
		break;
	case MVM_DEBUGFS_PM_SKIP_DTIM_PERIODS:
		IWL_DEBUG_POWER(mvm, "skip_dtim_periods=%d\n", val);
		dbgfs_pm->skip_dtim_periods = val;
		break;
	case MVM_DEBUGFS_PM_RX_DATA_TIMEOUT:
		IWL_DEBUG_POWER(mvm, "rx_data_timeout=%d\n", val);
		dbgfs_pm->rx_data_timeout = val;
		break;
	case MVM_DEBUGFS_PM_TX_DATA_TIMEOUT:
		IWL_DEBUG_POWER(mvm, "tx_data_timeout=%d\n", val);
		dbgfs_pm->tx_data_timeout = val;
		break;
	case MVM_DEBUGFS_PM_LPRX_ENA:
		IWL_DEBUG_POWER(mvm, "lprx %s\n", val ? "enabled" : "disabled");
		dbgfs_pm->lprx_ena = val;
		break;
	case MVM_DEBUGFS_PM_LPRX_RSSI_THRESHOLD:
		IWL_DEBUG_POWER(mvm, "lprx_rssi_threshold=%d\n", val);
		dbgfs_pm->lprx_rssi_threshold = val;
		break;
	case MVM_DEBUGFS_PM_SNOOZE_ENABLE:
		IWL_DEBUG_POWER(mvm, "snooze_enable=%d\n", val);
		dbgfs_pm->snooze_ena = val;
		break;
	case MVM_DEBUGFS_PM_UAPSD_MISBEHAVING:
		IWL_DEBUG_POWER(mvm, "uapsd_misbehaving_enable=%d\n", val);
		dbgfs_pm->uapsd_misbehaving = val;
		break;
	case MVM_DEBUGFS_PM_USE_PS_POLL:
		IWL_DEBUG_POWER(mvm, "use_ps_poll=%d\n", val);
		dbgfs_pm->use_ps_poll = val;
		break;
	}
}

static ssize_t iwl_dbgfs_pm_params_write(struct ieee80211_vif *vif, char *buf,
					 size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	enum iwl_dbgfs_pm_mask param;
	int val, ret;

	if (!strncmp("keep_alive=", buf, 11)) {
		if (sscanf(buf + 11, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_KEEP_ALIVE;
	} else if (!strncmp("skip_over_dtim=", buf, 15)) {
		if (sscanf(buf + 15, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_SKIP_OVER_DTIM;
	} else if (!strncmp("skip_dtim_periods=", buf, 18)) {
		if (sscanf(buf + 18, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_SKIP_DTIM_PERIODS;
	} else if (!strncmp("rx_data_timeout=", buf, 16)) {
		if (sscanf(buf + 16, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_RX_DATA_TIMEOUT;
	} else if (!strncmp("tx_data_timeout=", buf, 16)) {
		if (sscanf(buf + 16, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_TX_DATA_TIMEOUT;
	} else if (!strncmp("lprx=", buf, 5)) {
		if (sscanf(buf + 5, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_LPRX_ENA;
	} else if (!strncmp("lprx_rssi_threshold=", buf, 20)) {
		if (sscanf(buf + 20, "%d", &val) != 1)
			return -EINVAL;
		if (val > POWER_LPRX_RSSI_THRESHOLD_MAX || val <
		    POWER_LPRX_RSSI_THRESHOLD_MIN)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_LPRX_RSSI_THRESHOLD;
	} else if (!strncmp("snooze_enable=", buf, 14)) {
		if (sscanf(buf + 14, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_SNOOZE_ENABLE;
	} else if (!strncmp("uapsd_misbehaving=", buf, 18)) {
		if (sscanf(buf + 18, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_UAPSD_MISBEHAVING;
	} else if (!strncmp("use_ps_poll=", buf, 12)) {
		if (sscanf(buf + 12, "%d", &val) != 1)
			return -EINVAL;
		param = MVM_DEBUGFS_PM_USE_PS_POLL;
	} else {
		return -EINVAL;
	}

	mutex_lock(&mvm->mutex);
	iwl_dbgfs_update_pm(mvm, vif, param, val);
	ret = iwl_mvm_power_update_mac(mvm);
	mutex_unlock(&mvm->mutex);

	return ret ?: count;
}

static ssize_t iwl_dbgfs_tx_pwr_lmt_read(struct file *file,
					 char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	char buf[64];
	int bufsz = sizeof(buf);
	int pos;

	pos = scnprintf(buf, bufsz, "bss limit = %d\n",
			vif->bss_conf.txpower);

	return simple_read_from_buffer(user_buf, count, ppos, buf, pos);
}

static ssize_t iwl_dbgfs_pm_params_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	char buf[512];
	int bufsz = sizeof(buf);
	int pos;

	pos = iwl_mvm_power_mac_dbgfs_read(mvm, vif, buf, bufsz);

	return simple_read_from_buffer(user_buf, count, ppos, buf, pos);
}

static ssize_t iwl_dbgfs_mac_params_read(struct file *file,
					 char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u8 ap_sta_id;
	struct ieee80211_chanctx_conf *chanctx_conf;
	char buf[512];
	int bufsz = sizeof(buf);
	int pos = 0;
	int i;

	mutex_lock(&mvm->mutex);

	ap_sta_id = mvmvif->deflink.ap_sta_id;

	switch (ieee80211_vif_type_p2p(vif)) {
	case NL80211_IFTYPE_ADHOC:
		pos += scnprintf(buf+pos, bufsz-pos, "type: ibss\n");
		break;
	case NL80211_IFTYPE_STATION:
		pos += scnprintf(buf+pos, bufsz-pos, "type: bss\n");
		break;
	case NL80211_IFTYPE_AP:
		pos += scnprintf(buf+pos, bufsz-pos, "type: ap\n");
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		pos += scnprintf(buf+pos, bufsz-pos, "type: p2p client\n");
		break;
	case NL80211_IFTYPE_P2P_GO:
		pos += scnprintf(buf+pos, bufsz-pos, "type: p2p go\n");
		break;
	case NL80211_IFTYPE_P2P_DEVICE:
		pos += scnprintf(buf+pos, bufsz-pos, "type: p2p dev\n");
		break;
	case NL80211_IFTYPE_NAN:
		pos += scnprintf(buf+pos, bufsz-pos, "type: NAN\n");
		break;
	default:
		break;
	}

	pos += scnprintf(buf+pos, bufsz-pos, "mac id/color: %d / %d\n",
			 mvmvif->id, mvmvif->color);
	pos += scnprintf(buf+pos, bufsz-pos, "bssid: %pM\n",
			 vif->bss_conf.bssid);
	pos += scnprintf(buf+pos, bufsz-pos, "Load: %d\n",
			 mvm->tcm.result.load[mvmvif->id]);
	pos += scnprintf(buf+pos, bufsz-pos, "QoS:\n");
	for (i = 0; i < ARRAY_SIZE(mvmvif->deflink.queue_params); i++)
		pos += scnprintf(buf+pos, bufsz-pos,
				 "\t%d: txop:%d - cw_min:%d - cw_max = %d - aifs = %d upasd = %d\n",
				 i, mvmvif->deflink.queue_params[i].txop,
				 mvmvif->deflink.queue_params[i].cw_min,
				 mvmvif->deflink.queue_params[i].cw_max,
				 mvmvif->deflink.queue_params[i].aifs,
				 mvmvif->deflink.queue_params[i].uapsd);

	if (vif->type == NL80211_IFTYPE_STATION &&
	    ap_sta_id != IWL_MVM_INVALID_STA) {
		struct iwl_mvm_sta *mvm_sta;

		mvm_sta = iwl_mvm_sta_from_staid_protected(mvm, ap_sta_id);
		if (mvm_sta) {
			pos += scnprintf(buf+pos, bufsz-pos,
					 "ap_sta_id %d - reduced Tx power %d\n",
					 ap_sta_id,
					 mvm_sta->bt_reduced_txpower);
		}
	}

	rcu_read_lock();
	chanctx_conf = rcu_dereference(vif->bss_conf.chanctx_conf);
	if (chanctx_conf)
		pos += scnprintf(buf+pos, bufsz-pos,
				 "idle rx chains %d, active rx chains: %d\n",
				 chanctx_conf->rx_chains_static,
				 chanctx_conf->rx_chains_dynamic);
	rcu_read_unlock();

	mutex_unlock(&mvm->mutex);

	return simple_read_from_buffer(user_buf, count, ppos, buf, pos);
}

static void iwl_dbgfs_update_bf(struct ieee80211_vif *vif,
				enum iwl_dbgfs_bf_mask param, int value)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_dbgfs_bf *dbgfs_bf = &mvmvif->dbgfs_bf;

	dbgfs_bf->mask |= param;

	switch (param) {
	case MVM_DEBUGFS_BF_ENERGY_DELTA:
		dbgfs_bf->bf_energy_delta = value;
		break;
	case MVM_DEBUGFS_BF_ROAMING_ENERGY_DELTA:
		dbgfs_bf->bf_roaming_energy_delta = value;
		break;
	case MVM_DEBUGFS_BF_ROAMING_STATE:
		dbgfs_bf->bf_roaming_state = value;
		break;
	case MVM_DEBUGFS_BF_TEMP_THRESHOLD:
		dbgfs_bf->bf_temp_threshold = value;
		break;
	case MVM_DEBUGFS_BF_TEMP_FAST_FILTER:
		dbgfs_bf->bf_temp_fast_filter = value;
		break;
	case MVM_DEBUGFS_BF_TEMP_SLOW_FILTER:
		dbgfs_bf->bf_temp_slow_filter = value;
		break;
	case MVM_DEBUGFS_BF_ENABLE_BEACON_FILTER:
		dbgfs_bf->bf_enable_beacon_filter = value;
		break;
	case MVM_DEBUGFS_BF_DEBUG_FLAG:
		dbgfs_bf->bf_debug_flag = value;
		break;
	case MVM_DEBUGFS_BF_ESCAPE_TIMER:
		dbgfs_bf->bf_escape_timer = value;
		break;
	case MVM_DEBUGFS_BA_ENABLE_BEACON_ABORT:
		dbgfs_bf->ba_enable_beacon_abort = value;
		break;
	case MVM_DEBUGFS_BA_ESCAPE_TIMER:
		dbgfs_bf->ba_escape_timer = value;
		break;
	}
}

static ssize_t iwl_dbgfs_bf_params_write(struct ieee80211_vif *vif, char *buf,
					 size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	enum iwl_dbgfs_bf_mask param;
	int value, ret = 0;

	if (!strncmp("bf_energy_delta=", buf, 16)) {
		if (sscanf(buf+16, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_ENERGY_DELTA_MIN ||
		    value > IWL_BF_ENERGY_DELTA_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_ENERGY_DELTA;
	} else if (!strncmp("bf_roaming_energy_delta=", buf, 24)) {
		if (sscanf(buf+24, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_ROAMING_ENERGY_DELTA_MIN ||
		    value > IWL_BF_ROAMING_ENERGY_DELTA_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_ROAMING_ENERGY_DELTA;
	} else if (!strncmp("bf_roaming_state=", buf, 17)) {
		if (sscanf(buf+17, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_ROAMING_STATE_MIN ||
		    value > IWL_BF_ROAMING_STATE_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_ROAMING_STATE;
	} else if (!strncmp("bf_temp_threshold=", buf, 18)) {
		if (sscanf(buf+18, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_TEMP_THRESHOLD_MIN ||
		    value > IWL_BF_TEMP_THRESHOLD_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_TEMP_THRESHOLD;
	} else if (!strncmp("bf_temp_fast_filter=", buf, 20)) {
		if (sscanf(buf+20, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_TEMP_FAST_FILTER_MIN ||
		    value > IWL_BF_TEMP_FAST_FILTER_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_TEMP_FAST_FILTER;
	} else if (!strncmp("bf_temp_slow_filter=", buf, 20)) {
		if (sscanf(buf+20, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_TEMP_SLOW_FILTER_MIN ||
		    value > IWL_BF_TEMP_SLOW_FILTER_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_TEMP_SLOW_FILTER;
	} else if (!strncmp("bf_enable_beacon_filter=", buf, 24)) {
		if (sscanf(buf+24, "%d", &value) != 1)
			return -EINVAL;
		if (value < 0 || value > 1)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_ENABLE_BEACON_FILTER;
	} else if (!strncmp("bf_debug_flag=", buf, 14)) {
		if (sscanf(buf+14, "%d", &value) != 1)
			return -EINVAL;
		if (value < 0 || value > 1)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_DEBUG_FLAG;
	} else if (!strncmp("bf_escape_timer=", buf, 16)) {
		if (sscanf(buf+16, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BF_ESCAPE_TIMER_MIN ||
		    value > IWL_BF_ESCAPE_TIMER_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BF_ESCAPE_TIMER;
	} else if (!strncmp("ba_escape_timer=", buf, 16)) {
		if (sscanf(buf+16, "%d", &value) != 1)
			return -EINVAL;
		if (value < IWL_BA_ESCAPE_TIMER_MIN ||
		    value > IWL_BA_ESCAPE_TIMER_MAX)
			return -EINVAL;
		param = MVM_DEBUGFS_BA_ESCAPE_TIMER;
	} else if (!strncmp("ba_enable_beacon_abort=", buf, 23)) {
		if (sscanf(buf+23, "%d", &value) != 1)
			return -EINVAL;
		if (value < 0 || value > 1)
			return -EINVAL;
		param = MVM_DEBUGFS_BA_ENABLE_BEACON_ABORT;
	} else {
		return -EINVAL;
	}

	mutex_lock(&mvm->mutex);
	iwl_dbgfs_update_bf(vif, param, value);
	if (param == MVM_DEBUGFS_BF_ENABLE_BEACON_FILTER && !value)
		ret = iwl_mvm_disable_beacon_filter(mvm, vif, 0);
	else
		ret = iwl_mvm_enable_beacon_filter(mvm, vif, 0);
	mutex_unlock(&mvm->mutex);

	return ret ?: count;
}

static ssize_t iwl_dbgfs_bf_params_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char buf[256];
	int pos = 0;
	const size_t bufsz = sizeof(buf);
	struct iwl_beacon_filter_cmd cmd = {
		IWL_BF_CMD_CONFIG_DEFAULTS,
		.bf_enable_beacon_filter =
			cpu_to_le32(IWL_BF_ENABLE_BEACON_FILTER_DEFAULT),
		.ba_enable_beacon_abort =
			cpu_to_le32(IWL_BA_ENABLE_BEACON_ABORT_DEFAULT),
	};

	iwl_mvm_beacon_filter_debugfs_parameters(vif, &cmd);
	if (mvmvif->bf_data.bf_enabled)
		cmd.bf_enable_beacon_filter = cpu_to_le32(1);
	else
		cmd.bf_enable_beacon_filter = 0;

	pos += scnprintf(buf+pos, bufsz-pos, "bf_energy_delta = %d\n",
			 le32_to_cpu(cmd.bf_energy_delta));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_roaming_energy_delta = %d\n",
			 le32_to_cpu(cmd.bf_roaming_energy_delta));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_roaming_state = %d\n",
			 le32_to_cpu(cmd.bf_roaming_state));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_temp_threshold = %d\n",
			 le32_to_cpu(cmd.bf_temp_threshold));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_temp_fast_filter = %d\n",
			 le32_to_cpu(cmd.bf_temp_fast_filter));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_temp_slow_filter = %d\n",
			 le32_to_cpu(cmd.bf_temp_slow_filter));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_enable_beacon_filter = %d\n",
			 le32_to_cpu(cmd.bf_enable_beacon_filter));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_debug_flag = %d\n",
			 le32_to_cpu(cmd.bf_debug_flag));
	pos += scnprintf(buf+pos, bufsz-pos, "bf_escape_timer = %d\n",
			 le32_to_cpu(cmd.bf_escape_timer));
	pos += scnprintf(buf+pos, bufsz-pos, "ba_escape_timer = %d\n",
			 le32_to_cpu(cmd.ba_escape_timer));
	pos += scnprintf(buf+pos, bufsz-pos, "ba_enable_beacon_abort = %d\n",
			 le32_to_cpu(cmd.ba_enable_beacon_abort));

	return simple_read_from_buffer(user_buf, count, ppos, buf, pos);
}

static ssize_t iwl_dbgfs_os_device_timediff_read(struct file *file,
						 char __user *user_buf,
						 size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u32 curr_gp2;
	u64 curr_os;
	s64 diff;
	char buf[64];
	const size_t bufsz = sizeof(buf);
	int pos = 0;

	mutex_lock(&mvm->mutex);
	iwl_mvm_get_sync_time(mvm, CLOCK_BOOTTIME, &curr_gp2, &curr_os, NULL);
	mutex_unlock(&mvm->mutex);

	do_div(curr_os, NSEC_PER_USEC);
	diff = curr_os - curr_gp2;
	pos += scnprintf(buf + pos, bufsz - pos, "diff=%lld\n", diff);

	return simple_read_from_buffer(user_buf, count, ppos, buf, pos);
}

static ssize_t iwl_dbgfs_low_latency_write(struct ieee80211_vif *vif, char *buf,
					   size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u8 value;
	int ret;

	ret = kstrtou8(buf, 0, &value);
	if (ret)
		return ret;
	if (value > 1)
		return -EINVAL;

	mutex_lock(&mvm->mutex);
	iwl_mvm_update_low_latency(mvm, vif, value, LOW_LATENCY_DEBUGFS);
	mutex_unlock(&mvm->mutex);

	return count;
}

static ssize_t
iwl_dbgfs_low_latency_force_write(struct ieee80211_vif *vif, char *buf,
				  size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u8 value;
	int ret;

	ret = kstrtou8(buf, 0, &value);
	if (ret)
		return ret;

	if (value > NUM_LOW_LATENCY_FORCE)
		return -EINVAL;

	mutex_lock(&mvm->mutex);
	if (value == LOW_LATENCY_FORCE_UNSET) {
		iwl_mvm_update_low_latency(mvm, vif, false,
					   LOW_LATENCY_DEBUGFS_FORCE);
		iwl_mvm_update_low_latency(mvm, vif, false,
					   LOW_LATENCY_DEBUGFS_FORCE_ENABLE);
	} else {
		iwl_mvm_update_low_latency(mvm, vif,
					   value == LOW_LATENCY_FORCE_ON,
					   LOW_LATENCY_DEBUGFS_FORCE);
		iwl_mvm_update_low_latency(mvm, vif, true,
					   LOW_LATENCY_DEBUGFS_FORCE_ENABLE);
	}
	mutex_unlock(&mvm->mutex);
	return count;
}

static ssize_t iwl_dbgfs_low_latency_read(struct file *file,
					  char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char format[] = "traffic=%d\ndbgfs=%d\nvcmd=%d\nvif_type=%d\n"
			"dbgfs_force_enable=%d\ndbgfs_force=%d\nactual=%d\n";

	/*
	 * all values in format are boolean so the size of format is enough
	 * for holding the result string
	 */
	char buf[sizeof(format) + 1] = {};
	int len;

	len = scnprintf(buf, sizeof(buf) - 1, format,
			!!(mvmvif->low_latency & LOW_LATENCY_TRAFFIC),
			!!(mvmvif->low_latency & LOW_LATENCY_DEBUGFS),
			!!(mvmvif->low_latency & LOW_LATENCY_VCMD),
			!!(mvmvif->low_latency & LOW_LATENCY_VIF_TYPE),
			!!(mvmvif->low_latency &
			   LOW_LATENCY_DEBUGFS_FORCE_ENABLE),
			!!(mvmvif->low_latency & LOW_LATENCY_DEBUGFS_FORCE),
			!!(mvmvif->low_latency_actual));
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t iwl_dbgfs_uapsd_misbehaving_read(struct file *file,
						char __user *user_buf,
						size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char buf[20];
	int len;

	len = sprintf(buf, "%pM\n", mvmvif->uapsd_misbehaving_ap_addr);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t iwl_dbgfs_uapsd_misbehaving_write(struct ieee80211_vif *vif,
						 char *buf, size_t count,
						 loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	bool ret;

	mutex_lock(&mvm->mutex);
	ret = mac_pton(buf, mvmvif->uapsd_misbehaving_ap_addr);
	mutex_unlock(&mvm->mutex);

	return ret ? count : -EINVAL;
}

static ssize_t iwl_dbgfs_rx_phyinfo_write(struct ieee80211_vif *vif, char *buf,
					  size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	struct ieee80211_bss_conf *link_conf;
	u16 value;
	int link_id, ret = -EINVAL;

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	mutex_lock(&mvm->mutex);

	mvm->dbgfs_rx_phyinfo = value;

	for_each_vif_active_link(vif, link_conf, link_id) {
		struct ieee80211_chanctx_conf *chanctx_conf;
		struct cfg80211_chan_def min_def;
		struct iwl_mvm_phy_ctxt *phy_ctxt;
		u8 chains_static, chains_dynamic;

		rcu_read_lock();
		chanctx_conf = rcu_dereference(link_conf->chanctx_conf);
		if (!chanctx_conf) {
			rcu_read_unlock();
			continue;
		}
		/* A command can't be sent with RCU lock held, so copy
		 * everything here and use it after unlocking
		 */
		min_def = chanctx_conf->min_def;
		chains_static = chanctx_conf->rx_chains_static;
		chains_dynamic = chanctx_conf->rx_chains_dynamic;
		rcu_read_unlock();

		phy_ctxt = mvmvif->link[link_id]->phy_ctxt;
		if (!phy_ctxt)
			continue;

		ret = iwl_mvm_phy_ctxt_changed(mvm, phy_ctxt, &min_def,
					       chains_static, chains_dynamic);
	}

	mutex_unlock(&mvm->mutex);

	return ret ?: count;
}

static ssize_t iwl_dbgfs_rx_phyinfo_read(struct file *file,
					 char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char buf[8];
	int len;

	len = scnprintf(buf, sizeof(buf), "0x%04x\n",
			mvmvif->mvm->dbgfs_rx_phyinfo);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static void iwl_dbgfs_quota_check(void *data, u8 *mac,
				  struct ieee80211_vif *vif)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	int *ret = data;

	if (mvmvif->dbgfs_quota_min)
		*ret = -EINVAL;
}

static ssize_t iwl_dbgfs_quota_min_write(struct ieee80211_vif *vif, char *buf,
					 size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u16 value;
	int ret;

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	if (value > 95)
		return -EINVAL;

	mutex_lock(&mvm->mutex);

	mvmvif->dbgfs_quota_min = 0;
	ieee80211_iterate_interfaces(mvm->hw, IEEE80211_IFACE_ITER_NORMAL,
				     iwl_dbgfs_quota_check, &ret);
	if (ret == 0) {
		mvmvif->dbgfs_quota_min = value;
		iwl_mvm_update_quotas(mvm, false, NULL);
	}
	mutex_unlock(&mvm->mutex);

	return ret ?: count;
}

static ssize_t iwl_dbgfs_quota_min_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char buf[10];
	int len;

	len = scnprintf(buf, sizeof(buf), "%d\n", mvmvif->dbgfs_quota_min);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t iwl_dbgfs_twt_setup_write(struct ieee80211_vif *vif, char *buf,
					 size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	struct iwl_dhc_twt_operation *dhc_twt_cmd;
	struct iwl_dhc_cmd *cmd;

	u32 twt_operation;
	u64 target_wake_time;
	u32 interval_exp;
	u32 interval_mantissa;
	u32 min_wake_duration;
	u8 trigger;
	u8 flow_type;
	u8 flow_id;
	u8 protection;
	u8 twt_request = 1;
	u8 broadcast = 0;
	u8 tenth_param;
	int ret;

	ret = sscanf(buf, "%u %llu %u %u %u %hhu %hhu %hhu %hhu %hhu",
		     &twt_operation, &target_wake_time, &interval_exp,
		     &interval_mantissa, &min_wake_duration, &trigger,
		     &flow_type, &flow_id, &protection, &tenth_param);

	// the new twt_request parameter is optional for station
	if ((ret != 9 && ret != 10) ||
	    (vif->type != NL80211_IFTYPE_STATION && tenth_param == 1))
		return -EINVAL;

	/*
	 * The 10th parameter:
	 * In STA mode - the TWT type (broadcast or individual)
	 * In AP mode - the role (0 responder, 1 requester, 2 unsolicited)
	 */
	if (ret == 10) {
		if (vif->type == NL80211_IFTYPE_STATION)
			broadcast = tenth_param;
		else
			twt_request = tenth_param;
	}

	cmd = kzalloc(sizeof(*cmd) + sizeof(*dhc_twt_cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	dhc_twt_cmd = (void *)cmd->data;
	dhc_twt_cmd->mac_id = cpu_to_le32(mvmvif->id);
	dhc_twt_cmd->twt_operation = cpu_to_le32(twt_operation);
	dhc_twt_cmd->target_wake_time = cpu_to_le64(target_wake_time);
	dhc_twt_cmd->interval_exp = cpu_to_le32(interval_exp);
	dhc_twt_cmd->interval_mantissa = cpu_to_le32(interval_mantissa);
	dhc_twt_cmd->min_wake_duration = cpu_to_le32(min_wake_duration);
	dhc_twt_cmd->trigger = trigger;
	dhc_twt_cmd->flow_type = flow_type;
	dhc_twt_cmd->flow_id = flow_id;
	dhc_twt_cmd->protection = protection;
	dhc_twt_cmd->twt_request = twt_request;
	dhc_twt_cmd->negotiation_type = broadcast ? 3 : 0;

	cmd->length = cpu_to_le32(sizeof(*dhc_twt_cmd) >> 2);
	cmd->index_and_mask =
		cpu_to_le32(DHC_TABLE_INTEGRATION | DHC_TARGET_UMAC |
			    DHC_INT_UMAC_TWT_OPERATION);

	mutex_lock(&mvm->mutex);
	ret = iwl_mvm_send_cmd_pdu(mvm,
				   WIDE_ID(IWL_ALWAYS_LONG_GROUP, DEBUG_HOST_COMMAND),
				   0, sizeof(*cmd) + sizeof(*dhc_twt_cmd),
				   cmd);
	mutex_unlock(&mvm->mutex);
	kfree(cmd);

	return ret ?: count;
}

static ssize_t iwl_dbgfs_eht_puncturing_read(struct file *file,
					     char __user *user_buf,
					     size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	char buf[8];
	int len;

	len = scnprintf(buf, sizeof(buf), "0x%04x\n",
			vif->bss_conf.eht_puncturing);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t iwl_dbgfs_eht_puncturing_write(struct ieee80211_vif *vif, char *buf,
					      size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	struct ieee80211_sta *sta;
	struct iwl_mvm_sta *mvmsta;
	u16 puncturing;
	int ret, i;

	if (!iwl_mvm_firmware_running(mvm))
		return -EIO;

	if (vif->type != NL80211_IFTYPE_AP)
		return -EINVAL;

	ret = kstrtou16(buf, 0, &puncturing);
	if (ret)
		return ret;

	/*
	 * MVM is not supposed to modify the BSS info,
	 * but softAP doesn't use this field anyway.
	 */
	vif->bss_conf.eht_puncturing = puncturing;

	mutex_lock(&mvm->mutex);

	for (i = 0; i < mvm->fw->ucode_capa.num_stations; i++) {
		sta = rcu_dereference_protected(mvm->fw_id_to_mac_id[i],
						lockdep_is_held(&mvm->mutex));

		if (IS_ERR_OR_NULL(sta))
			continue;

		mvmsta = iwl_mvm_sta_from_mac80211(sta);
		if (mvmsta->vif != vif)
			continue;

		iwl_mvm_cfg_he_sta(mvm, vif, mvmsta->deflink.sta_id);
	}

	mutex_unlock(&mvm->mutex);
	return count;
}

static ssize_t iwl_dbgfs_max_tx_op_write(struct ieee80211_vif *vif, char *buf,
					 size_t count, loff_t *ppos)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	u16 value;
	int ret;

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	mutex_lock(&mvm->mutex);
	mvmvif->max_tx_op = value;
	mutex_unlock(&mvm->mutex);

	return count;
}

static ssize_t iwl_dbgfs_max_tx_op_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ieee80211_vif *vif = file->private_data;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_mvm *mvm = mvmvif->mvm;
	char buf[10];
	int len;

	mutex_lock(&mvm->mutex);
	len = scnprintf(buf, sizeof(buf), "%hu\n", mvmvif->max_tx_op);
	mutex_unlock(&mvm->mutex);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

#define MVM_DEBUGFS_WRITE_FILE_OPS(name, bufsz) \
	_MVM_DEBUGFS_WRITE_FILE_OPS(name, bufsz, struct ieee80211_vif)
#define MVM_DEBUGFS_READ_WRITE_FILE_OPS(name, bufsz) \
	_MVM_DEBUGFS_READ_WRITE_FILE_OPS(name, bufsz, struct ieee80211_vif)
#define MVM_DEBUGFS_ADD_FILE_VIF(name, parent, mode) do {		\
		debugfs_create_file(#name, mode, parent, vif,		\
				    &iwl_dbgfs_##name##_ops);		\
	} while (0)

MVM_DEBUGFS_READ_FILE_OPS(mac_params);
MVM_DEBUGFS_READ_FILE_OPS(tx_pwr_lmt);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(pm_params, 32);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(bf_params, 256);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(low_latency, 10);
MVM_DEBUGFS_WRITE_FILE_OPS(low_latency_force, 10);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(uapsd_misbehaving, 20);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(rx_phyinfo, 10);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(quota_min, 32);
MVM_DEBUGFS_READ_FILE_OPS(os_device_timediff);
MVM_DEBUGFS_WRITE_FILE_OPS(twt_setup, 256);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(eht_puncturing, 16);
MVM_DEBUGFS_READ_WRITE_FILE_OPS(max_tx_op, 10);


void iwl_mvm_vif_dbgfs_register(struct iwl_mvm *mvm, struct ieee80211_vif *vif)
{
	struct dentry *dbgfs_dir = vif->debugfs_dir;
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	char buf[100];

	/*
	 * Check if debugfs directory already exist before creating it.
	 * This may happen when, for example, resetting hw or suspend-resume
	 */
	if (!dbgfs_dir || mvmvif->dbgfs_dir)
		return;

	mvmvif->dbgfs_dir = debugfs_create_dir("iwlmvm", dbgfs_dir);
	if (IS_ERR_OR_NULL(mvmvif->dbgfs_dir)) {
		IWL_ERR(mvm, "Failed to create debugfs directory under %pd\n",
			dbgfs_dir);
		return;
	}

	if (iwlmvm_mod_params.power_scheme != IWL_POWER_SCHEME_CAM &&
	    ((vif->type == NL80211_IFTYPE_STATION && !vif->p2p) ||
	     (vif->type == NL80211_IFTYPE_STATION && vif->p2p)))
		MVM_DEBUGFS_ADD_FILE_VIF(pm_params, mvmvif->dbgfs_dir, 0600);

	MVM_DEBUGFS_ADD_FILE_VIF(tx_pwr_lmt, mvmvif->dbgfs_dir, 0400);
	MVM_DEBUGFS_ADD_FILE_VIF(mac_params, mvmvif->dbgfs_dir, 0400);
	MVM_DEBUGFS_ADD_FILE_VIF(low_latency, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(low_latency_force, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(uapsd_misbehaving, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(rx_phyinfo, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(quota_min, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(os_device_timediff, mvmvif->dbgfs_dir, 0400);
	MVM_DEBUGFS_ADD_FILE_VIF(twt_setup, mvmvif->dbgfs_dir, 0200);
	MVM_DEBUGFS_ADD_FILE_VIF(eht_puncturing, mvmvif->dbgfs_dir, 0600);
	MVM_DEBUGFS_ADD_FILE_VIF(max_tx_op, mvmvif->dbgfs_dir, 0600);

	if (vif->type == NL80211_IFTYPE_STATION && !vif->p2p &&
	    mvmvif == mvm->bf_allowed_vif)
		MVM_DEBUGFS_ADD_FILE_VIF(bf_params, mvmvif->dbgfs_dir, 0600);

	/*
	 * Create symlink for convenience pointing to interface specific
	 * debugfs entries for the driver. For example, under
	 * /sys/kernel/debug/iwlwifi/0000\:02\:00.0/iwlmvm/
	 * find
	 * netdev:wlan0 -> ../../../ieee80211/phy0/netdev:wlan0/iwlmvm/
	 */
	snprintf(buf, 100, "../../../%pd3/%pd",
		 dbgfs_dir,
		 mvmvif->dbgfs_dir);

	mvmvif->dbgfs_slink = debugfs_create_symlink(dbgfs_dir->d_name.name,
						     mvm->debugfs_dir, buf);
}

void iwl_mvm_vif_dbgfs_clean(struct iwl_mvm *mvm, struct ieee80211_vif *vif)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);

	debugfs_remove(mvmvif->dbgfs_slink);
	mvmvif->dbgfs_slink = NULL;

	debugfs_remove_recursive(mvmvif->dbgfs_dir);
	mvmvif->dbgfs_dir = NULL;
}
