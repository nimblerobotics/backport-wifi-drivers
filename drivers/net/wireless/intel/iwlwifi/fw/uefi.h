/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright(c) 2021-2023 Intel Corporation
 */
#ifndef __iwl_fw_uefi__
#define __iwl_fw_uefi__

#include "fw/regulatory.h"

#define IWL_UEFI_OEM_PNVM_NAME		L"UefiCnvWlanOemSignedPnvm"
#define IWL_UEFI_REDUCED_POWER_NAME	L"UefiCnvWlanReducedPower"
#define IWL_UEFI_SGOM_NAME		L"UefiCnvWlanSarGeoOffsetMapping"
#define IWL_UEFI_STEP_NAME		L"UefiCnvCommonSTEP"
#define IWL_UEFI_UATS_NAME		L"CnvUefiWlanUATS"
#define IWL_UEFI_WRDS_NAME		L"UefiCnvWlanWRDS"
#define IWL_UEFI_EWRD_NAME		L"UefiCnvWlanEWRD"
#define IWL_UEFI_WGDS_NAME		L"UefiCnvWlanWGDS"

#define IWL_SGOM_MAP_SIZE		339
#define IWL_UATS_MAP_SIZE		339

#define IWL_UEFI_WRDS_REVISION		2
#define IWL_UEFI_EWRD_REVISION		2
#define IWL_UEFI_WGDS_REVISION		3

struct pnvm_sku_package {
	u8 rev;
	u32 total_size;
	u8 n_skus;
	u32 reserved[2];
	u8 data[];
} __packed;

struct uefi_cnv_wlan_sgom_data {
	u8 revision;
	u8 offset_map[IWL_SGOM_MAP_SIZE - 1];
} __packed;

struct uefi_cnv_wlan_uats_data {
	u8 revision;
	u8 offset_map[IWL_UATS_MAP_SIZE - 1];
} __packed;

struct uefi_cnv_common_step_data {
	u8 revision;
	u8 step_mode;
	u8 cnvi_eq_channel;
	u8 cnvr_eq_channel;
	u8 radio1;
	u8 radio2;
} __packed;

/*
 * struct uefi_sar_profile - a SAR profile as defined in UEFI
 *
 * @chains: a per-chain table of SAR values
 */
struct uefi_sar_profile {
	struct iwl_sar_profile_chain chains[BIOS_SAR_MAX_CHAINS_PER_PROFILE];
} __packed;

/*
 * struct uefi_cnv_var_wrds - WRDS table as defined in UEFI
 *
 * @revision: the revision of the table
 * @mode: is WRDS enbaled/disabled
 * @sar_profile: sar profile #1
 */
struct uefi_cnv_var_wrds {
	u8 revision;
	u32 mode;
	struct uefi_sar_profile sar_profile;
} __packed;

/*
 * struct uefi_cnv_var_ewrd - EWRD table as defined in UEFI
 * @revision: the revision of the table
 * @mode: is WRDS enbaled/disabled
 * @num_profiles: how many additional profiles we have in this table (0-3)
 * @sar_profiles: the additional SAR profiles (#2-#4)
 */
struct uefi_cnv_var_ewrd {
	u8 revision;
	u32 mode;
	u32 num_profiles;
	struct uefi_sar_profile sar_profiles[BIOS_SAR_MAX_PROFILE_NUM - 1];
} __packed;

/*
 * struct uefi_cnv_var_wgds - WGDS table as defined in UEFI
 * @revision: the revision of the table
 * @num_profiles: the number of geo profiles we have in the table.
 *	The first 3 are mandatory, and can have up to 8.
 * @geo_profiles: a per-profile table of the offsets to add to SAR values.
 */
struct uefi_cnv_var_wgds {
	u8 revision;
	u8 num_profiles;
	struct iwl_geo_profile geo_profiles[BIOS_GEO_MAX_PROFILE_NUM];
} __packed;

/*
 * This is known to be broken on v4.19 and to work on v5.4.  Until we
 * figure out why this is the case and how to make it work, simply
 * disable the feature in old kernels.
 */
#if defined(CONFIG_EFI) && LINUX_VERSION_IS_GEQ(5,4,0)
void *iwl_uefi_get_pnvm(struct iwl_trans *trans, size_t *len);
u8 *iwl_uefi_get_reduced_power(struct iwl_trans *trans, size_t *len);
int iwl_uefi_reduce_power_parse(struct iwl_trans *trans,
				const u8 *data, size_t len,
				struct iwl_pnvm_image *pnvm_data);
void iwl_uefi_get_step_table(struct iwl_trans *trans);
int iwl_uefi_handle_tlv_mem_desc(struct iwl_trans *trans, const u8 *data,
				 u32 tlv_len, struct iwl_pnvm_image *pnvm_data);
int iwl_uefi_get_wrds_table(struct iwl_fw_runtime *fwrt);
int iwl_uefi_get_ewrd_table(struct iwl_fw_runtime *fwrt);
int iwl_uefi_get_wgds_table(struct iwl_fw_runtime *fwrt);
#else /* CONFIG_EFI */
static inline void *iwl_uefi_get_pnvm(struct iwl_trans *trans, size_t *len)
{
	return ERR_PTR(-EOPNOTSUPP);
}

static inline int
iwl_uefi_reduce_power_parse(struct iwl_trans *trans,
			    const u8 *data, size_t len,
			    struct iwl_pnvm_image *pnvm_data)
{
	return -EOPNOTSUPP;
}

static inline u8 *
iwl_uefi_get_reduced_power(struct iwl_trans *trans, size_t *len)
{
	return ERR_PTR(-EOPNOTSUPP);
}

static inline void iwl_uefi_get_step_table(struct iwl_trans *trans)
{
}

static inline int
iwl_uefi_handle_tlv_mem_desc(struct iwl_trans *trans, const u8 *data,
			     u32 tlv_len, struct iwl_pnvm_image *pnvm_data)
{
	return 0;
}

static inline int iwl_uefi_get_wrds_table(struct iwl_fw_runtime *fwrt)
{
	return -ENOENT;
}

static inline int iwl_uefi_get_ewrd_table(struct iwl_fw_runtime *fwrt)
{
	return -ENOENT;
}

static inline int iwl_uefi_get_wgds_table(struct iwl_fw_runtime *fwrt)
{
	return -ENOENT;
}
#endif /* CONFIG_EFI */

#if defined(CONFIG_EFI) && defined(CONFIG_ACPI) && LINUX_VERSION_IS_GEQ(5,4,0)
void iwl_uefi_get_sgom_table(struct iwl_trans *trans, struct iwl_fw_runtime *fwrt);
int iwl_uefi_get_uats_table(struct iwl_trans *trans,
			    struct iwl_fw_runtime *fwrt);
#else
static inline
void iwl_uefi_get_sgom_table(struct iwl_trans *trans, struct iwl_fw_runtime *fwrt)
{
}

static inline
int iwl_uefi_get_uats_table(struct iwl_trans *trans,
			    struct iwl_fw_runtime *fwrt)
{
	return 0;
}
#endif
#endif /* __iwl_fw_uefi__ */
