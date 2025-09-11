/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef __RULE_BASED_EXE_H__
#define __RULE_BASED_EXE_H__

#include "include/acs_val.h"
#include "include/acs_common.h"
#include "include/val_interface.h"

#define RULE_STRING_SIZE 10
#define RULE_DESC_SIZE   40
#define INVALID_IDX 0xFFFFFFFF

/* ----------------------------  Enum Definations -----------------------------------------------*/

/* Rule ID enum definition */
/* Module wise sorted, so that tests can be run module wise without additional metadata check
   and sorting */
typedef enum {
    /* ACS alias */
    S_L3_01,
    P_L1_01,
    /* PE BSA rules */
    B_PE_01,
    B_PE_02,
    B_PE_03,
    B_PE_04,
    B_PE_05,
    B_PE_06,
    B_PE_07,
    B_PE_08,
    B_PE_09,
    B_PE_10,
    B_PE_11,
    B_PE_12,
    B_PE_13,
    B_PE_14,
    B_PE_15,
    B_PE_16,
    B_PE_17,
    B_PE_18,
    B_PE_19,
    B_PE_20,
    B_PE_21,
    B_PE_22,
    B_PE_23,
    B_PE_24,
    B_PE_25,
    B_SEC_01,
    B_SEC_02,
    B_SEC_03,
    B_SEC_04,
    B_SEC_05,

    /* PE SBSA rules */
    S_L3PE_01,
    S_L3PE_02,
    S_L3PE_03,
    S_L3PE_04,
    S_L4PE_01,
    S_L4PE_02,
    S_L4PE_03,
    S_L4PE_04,
    S_L5PE_01,
    S_L5PE_02,
    S_L5PE_03,
    S_L5PE_04,
    S_L5PE_05,
    S_L5PE_06,
    S_L5PE_07,
    S_L6PE_01,
    S_L6PE_02,
    S_L6PE_03,
    S_L6PE_04,
    S_L6PE_05,
    S_L6PE_06,
    S_L6PE_07,
    S_L6PE_08,
    S_L7PE_01,
    S_L7PE_02,
    S_L7PE_04,
    S_L7PE_05,
    S_L7PE_06,
    S_L7PE_07,
    S_L7PE_08,
    S_L7PE_09,
    S_L7TME_1,
    S_L7TME_2,
    S_L7TME_3,
    S_L7TME_4,
    S_L7TME_5,
    S_L8PE_01,
    S_L8PE_02,
    S_L8PE_03,
    S_L8PE_04,
    S_L8PE_05,
    S_L8PE_06,
    S_L8PE_07,
    S_L8PE_08,
    S_L8RME_1,
    S_L8SHD_1,

    /* PE PCBSA rules */
    P_L1PE_01,
    P_L1PE_02,
    P_L1PE_03,
    P_L1PE_04,
    P_L1PE_05,
    P_L1PE_06,
    P_L1PE_07,
    P_L1PE_08,

    /* GIC rules */
    B_GIC_01,
    B_GIC_02,
    B_GIC_03,
    B_GIC_04,
    B_GIC_05,
    ITS_01,
    ITS_02,
    ITS_DEV_2,
    ITS_DEV_7,
    ITS_DEV_8,
    S_L3GI_01,
    B_PPI_00,
    S_L3PP_01,
    S_L5GI_01,
    S_L5PP_01,
    S_L8GI_01,
    Appendix_I_5,   /* Appendix I.5 remapped to a legal identifier */
    Appendix_I_6,   /* Appendix I.6 remapped */
    Appendix_I_9,   /* Appendix I.9 remapped */
    P_L1GI_01,
    P_L1GI_02,
    P_L1GI_03,
    P_L1GI_04,
    P_L1PP_01,

    /* PERIPHERAL rules */
    B_PER_01,
    B_PER_02,
    B_PER_03,
    B_PER_04,
    B_PER_05,
    S_L3PR_01,
    B_PER_06,
    B_PER_07,
    B_PER_08,
    B_PER_09,
    B_PER_10,
    B_PER_11,

    /* MEM_MAP rules */
    B_MEM_01,
    B_MEM_02,
    B_MEM_03,
    B_MEM_04,
    B_MEM_05,
    B_MEM_06,
    B_MEM_07,
    B_MEM_08,
    B_MEM_09,
    S_L3MM_01,
    S_L3MM_02,
    P_L1MM_01,

    /* PMU rules */
    PMU_PE_02,
    PMU_PE_03,
    PMU_SYS_1,
    PMU_SYS_2,
    PMU_SYS_5,
    PMU_SYS_6,
    PMU_BM_1,
    PMU_MEM_1,
    PMU_BM_2,
    PMU_EV_11,
    PMU_SPE,
    S_L7PMU,

    /* RAS rules */
    RAS_01,
    RAS_02,
    RAS_03,
    RAS_04,
    RAS_06,
    RAS_07,
    RAS_08,
    RAS_11,
    S_L7RAS_1,
    S_L7RAS_2,
    S_RAS_01,
    S_RAS_03,
    SYS_RAS,
    SYS_RAS_1,
    SYS_RAS_2,
    SYS_RAS_3,
    SYS_RAS_4,

    /* SMMU rules */
    B_SMMU_01,
    B_SMMU_02,
    B_SMMU_03,
    B_SMMU_04,
    B_SMMU_05,
    B_SMMU_06,
    B_SMMU_07,
    B_SMMU_08,
    B_SMMU_09,
    B_SMMU_11,
    B_SMMU_12,
    B_SMMU_13,
    B_SMMU_14,
    B_SMMU_16,
    B_SMMU_17,
    B_SMMU_18,
    B_SMMU_19,
    B_SMMU_20,
    B_SMMU_21,
    B_SMMU_23,
    B_SMMU_24,
    B_SMMU_25,
    GPU_04, // TODO multiple module tests
    SMMU_01,
    S_L3SM_01,

    S_L4SM_01,
    S_L4SM_02,
    S_L4SM_03,
    S_L5SM_01,
    S_L5SM_02,
    S_L5SM_03,
    S_L5SM_04,
    S_L6SM_02,
    S_L6SM_03,
    S_L6SM_04,
    S_L7SM_01,
    S_L7SM_02,
    S_L7SM_03,
    S_L7SM_04,
    S_L8SM_01,

    P_L1SM_01,
    P_L1SM_02,
    P_L1SM_03,
    P_L1SM_04,
    P_L1SM_05,

    /* TIMER rules */
    B_TIME_01,
    B_TIME_02,
    B_TIME_03,
    B_TIME_04,
    B_TIME_05,
    B_TIME_06,
    B_TIME_07,
    B_TIME_08,
    B_TIME_09,
    B_TIME_10,
    S_L5TI_01,
    S_L8TI_01,

    /* WATCHDOG rules */
    B_WD_00,
    B_WD_01,
    B_WD_02,
    B_WD_03,
    B_WD_04,
    B_WD_05,
    S_L3WD_01,
    S_L6WD_01,
    P_L2WD_01,

    /* NIST / ENTROPY rules */
    S_L7ENT_1,

    /* MPAM */
    S_L7MP_01,
    S_L7MP_02,
    S_L7MP_03,
    S_L7MP_04,
    S_L7MP_05,
    S_L7MP_07,
    S_L7MP_08,

    /* ETE */
    ETE_02,
    ETE_03,
    ETE_04,
    ETE_05,
    ETE_06,
    ETE_07,
    ETE_08,
    ETE_09,
    ETE_10,

    /* Power wakeup */
    B_WAK_01,
    B_WAK_02,
    B_WAK_03,
    B_WAK_04,
    B_WAK_05,
    B_WAK_06,
    B_WAK_07,
    B_WAK_08,
    B_WAK_10,
    B_WAK_11,

    /* NVStore/Security */
    P_L1NV_01,
    P_L1SE_01,
    P_L1SE_02,
    P_L1SE_03,
    P_L1SE_04,
    P_L1SE_05,

    /* TPM */
    P_L1TP_01,
    P_L1TP_02,
    P_L1TP_03,
    P_L1TP_04,

    /*PCIe*/
    B_PER_12,  // revisit peripheral rule in pcie module
    B_PCIe_10,
    B_PCIe_11,
    GPU_01,
    GPU_02,
    GPU_03,
    B_REP_1,
    B_IEP_1,
    BJLPB,
    XRPZG,
    IE_ACS_1,
    IE_ACS_2,
    IE_BAR_1,
    IE_BAR_3,
    IE_INT_1,
    IE_ORD_1,
    IE_ORD_4,
    IE_PWR_1,
    IE_REG_1,
    IE_REG_2,
    IE_REG_3,
    IE_REG_4,
    IE_REG_5,
    IE_REG_6,
    IE_REG_7,
    IE_REG_8,
    IE_REG_9,
    IE_RST_1,
    IE_RST_2,
    IE_SMU_1,
    IE_SMU_3,
    ITS_03,
    ITS_04,
    ITS_05,
    ITS_DEV_4,
    ITS_DEV_6,
    PCI_ER_01,
    PCI_ER_04,
    PCI_ER_05,
    PCI_ER_06,
    PCI_ER_07,
    PCI_ER_08,
    PCI_ER_09,
    PCI_ER_10,
    PCI_IC_11,
    PCI_IC_15,
    PCI_IN_01,
    PCI_IN_02,
    PCI_IN_03,
    PCI_IN_04,
    PCI_IN_05,
    PCI_IN_11,
    PCI_IN_12,
    PCI_IN_13,
    PCI_IN_16,
    PCI_IN_17,
    PCI_IN_18,
    PCI_IN_19,
    PCI_IN_20,
    PCI_LI_01,
    PCI_LI_02,
    PCI_LI_03,
    PCI_MM_01,
    PCI_MM_03,
    PCI_MM_04,
    PCI_MM_05,
    PCI_MM_07,
    PCI_MSI_01,
    PCI_MSI_2,
    PCI_PAS_1,
    PCI_PP_02,
    PCI_PP_03,
    PCI_PP_04,
    PCI_PP_05,
    PCI_SM_02,
    RE_ACS_1,
    RE_ACS_2,
    RE_BAR_1,
    RE_BAR_3,
    RE_INT_1,
    RE_ORD_1,
    RE_ORD_4,
    RE_PCI_1,
    RE_PCI_2,
    RE_PWR_1,
    RE_REC_1,
    RE_REG_1,
    RE_REG_2,
    RE_REG_3,
    RE_RST_1,
    RE_SMU_2,
    RE_SMU_4,
    S_L4PCI_1,
    S_L4PCI_2,
    S_L6PCI_1,
    S_PCIe_01,
    S_PCIe_02,
    S_PCIe_03,
    S_PCIe_04,
    S_PCIe_05,
    S_PCIe_06,
    S_PCIe_09,
    S_PCIe_07,
    S_PCIe_08,
    S_PCIe_10,
    S_PCIe_11,
    S_L8CXL_1,
    S_L3GI_02, /* GIC rule is in part of PCIe module */

    P_L1PCI_1,
    P_L1PCI_2,

    /* Sentinel must remain last */
    RULE_ID_SENTINEL
} RULE_ID_e;

/* Enum for test entry IDs
 *    (Names are entry function names uppercased) */
typedef enum {
    NULL_ENTRY,
    PE001_ENTRY,
    PE002_ENTRY,
    PE003_ENTRY,
    PE004_ENTRY,
    PE006_ENTRY,
    PE007_ENTRY,
    PE008_ENTRY,
    PE009_ENTRY,
    PE010_ENTRY,
    PE011_ENTRY,
    PE012_ENTRY,
    PE013_ENTRY,
    PE016_ENTRY,
    PE014_ENTRY,
    PE017_ENTRY,
    PE018_ENTRY,
    PE019_ENTRY,
    PE020_ENTRY,
    PE021_ENTRY,
    PE022_ENTRY,
    PE063_ENTRY,
    PE037_ENTRY,
    PE015_ENTRY,
    PE043_ENTRY,
    PE044_ENTRY,
    PE045_ENTRY,
    PE046_ENTRY,
    PE047_ENTRY,
    PE023_ENTRY,
    PE024_ENTRY,
    PE025_ENTRY,
    PE026_ENTRY,
    PE027_ENTRY,
    PE028_ENTRY,
    PE029_ENTRY,
    PE030_ENTRY,
    PE031_ENTRY,
    PE032_ENTRY,
    PE033_ENTRY,
    PE034_ENTRY,
    PE035_ENTRY,
    PE036_ENTRY,
    PE038_ENTRY,
    PE039_ENTRY,
    PE040_ENTRY,
    PE041_ENTRY,
    PE042_ENTRY,
    PE048_ENTRY,
    PE049_ENTRY,
    PE051_ENTRY,
    PE052_ENTRY,
    PE053_ENTRY,
    PE054_ENTRY,
    PE058_ENTRY,
    PE059_ENTRY,
    PE060_ENTRY,
    PE057_ENTRY,
    PE064_ENTRY,
    PE061_ENTRY,
    PE062_ENTRY,
    G001_ENTRY,
    G002_ENTRY,
    G003_ENTRY,
    G004_ENTRY,
    G005_ENTRY,
    ITS001_ENTRY,
    ITS002_ENTRY,
    ITS003_ENTRY,
    ITS004_ENTRY,
    ITS005_ENTRY,
    G006_ENTRY,
    G007_ENTRY,
    G009_ENTRY,
    G010_ENTRY,
    G011_ENTRY,
    G012_ENTRY,
    B_PPI_00_ENTRY,
    G014_ENTRY,
    G016_ENTRY,
    G013_ENTRY,
    G015_ENTRY,
    V2M004_ENTRY,
    V2M001_ENTRY,
    APPENDIX_I_6_ENTRY,
    V2M002_ENTRY,
    V2M003_ENTRY,
    D001_ENTRY,
    D008_ENTRY,
    D002_ENTRY,
    D003_ENTRY,
    D006_ENTRY,
    D004_ENTRY,
    D007_ENTRY,
    M002_ENTRY,
    M001_ENTRY,
    M004_ENTRY,
    M006_ENTRY,
    M007_ENTRY,
    M003_ENTRY,
    M005_ENTRY,
    M008_ENTRY,
    PMU001_ENTRY,
    PMU002_ENTRY,
    PMU004_ENTRY,
    PMU005_ENTRY,
    PMU009_ENTRY,
    PMU007_ENTRY,
    PMU010_ENTRY,
    PMU011_ENTRY,
    PMU003_ENTRY,
    PMU006_ENTRY,
    RAS001_ENTRY,
    RAS002_ENTRY,
    RAS003_ENTRY,
    RAS004_ENTRY,
    RAS005_ENTRY,
    RAS006_ENTRY,
    RAS007_ENTRY,
    RAS008_ENTRY,
    RAS009_ENTRY,
    RAS014_ENTRY,
    RAS010_ENTRY,
    RAS011_ENTRY,
    RAS012_ENTRY,
    SYS_RAS_2_ENTRY,
    RAS015_ENTRY,
    RAS013_ENTRY,
    I001_ENTRY,
    I007_ENTRY,
    I002_ENTRY,
    I016_ENTRY,
    I017_ENTRY,
    I030_ENTRY,
    INTERFACE010_ENTRY,
    INTERFACE011_ENTRY,
    I003_ENTRY,
    I004_ENTRY,
    I010_ENTRY,
    I012_ENTRY,
    I018_ENTRY,
    I019_ENTRY,
    I005_ENTRY,
    I029_ENTRY,
    I006_ENTRY,
    I011_ENTRY,
    I015_ENTRY,
    I024_ENTRY,
    I008_ENTRY,
    I025_ENTRY,
    I020_ENTRY,
    I009_ENTRY,
    I026_ENTRY,
    I013_ENTRY,
    I014_ENTRY,
    I022_ENTRY,
    I023_ENTRY,
    I021_ENTRY,
    I027_ENTRY,
    I028_ENTRY,
    T001_ENTRY,
    T007_ENTRY,
    T002_ENTRY,
    T003_ENTRY,
    T004_ENTRY,
    T005_ENTRY,
    T006_ENTRY,
    W001_ENTRY,
    W002_ENTRY,
    W003_ENTRY,
    N001_ENTRY,
    U001_ENTRY,
    U002_ENTRY,
    U003_ENTRY,
    U004_ENTRY,
    U005_ENTRY,
    U006_ENTRY,
    B_WAK_03_07_ENTRY,
    P_L1PE_01_ENTRY,
    P021_ENTRY,
    P093_ENTRY,
    P089_ENTRY,
    P082_ENTRY,
    P081_ENTRY,
    P029_ENTRY,
    P041_ENTRY,
    P055_ENTRY,
    E031_ENTRY,
    E038_ENTRY,
    P034_ENTRY,
    IE_REG_1_ENTRY,
    IE_REG_2_ENTRY,
    IE_REG_3_ENTRY,
    IE_REG_4_ENTRY,
    P057_ENTRY,
    P092_ENTRY,
    P012_ENTRY,
    P013_ENTRY,
    P014_ENTRY,
    P040_ENTRY,
    P079_ENTRY,
    P080_ENTRY,
    E037_ENTRY,
    E011_ENTRY,
    E035_ENTRY,
    E012_ENTRY,
    E013_ENTRY,
    E004_ENTRY,
    P010_ENTRY,
    E023_ENTRY,
    P007_ENTRY,
    E024_ENTRY,
    E029_ENTRY,
    E028_ENTRY,
    P090_ENTRY,
    E027_ENTRY,
    PCI_IC_11_ENTRY,
    E001_ENTRY,
    E002_ENTRY,
    E003_ENTRY,
    P001_ENTRY,
    P002_ENTRY,
    P003_ENTRY,
    P004_ENTRY,
    P005_ENTRY,
    P006_ENTRY,
    P038_ENTRY,
    P017_ENTRY,
    P018_ENTRY,
    P023_ENTRY,
    P027_ENTRY,
    P030_ENTRY,
    P031_ENTRY,
    P032_ENTRY,
    P048_ENTRY,
    P051_ENTRY,
    P053_ENTRY,
    P054_ENTRY,
    P058_ENTRY,
    P061_ENTRY,
    P065_ENTRY,
    P066_ENTRY,
    P067_ENTRY,
    P072_ENTRY,
    P078_ENTRY,
    P088_ENTRY,
    P098_ENTRY,
    P099_ENTRY,
    PCI_IN_04_ENTRY,
    PCI_IN_05_ENTRY,
    E010_ENTRY,
    P037_ENTRY,
    PCI_IN_13_ENTRY,
    P008_ENTRY,
    PCI_IN_17_ENTRY,
    P011_ENTRY,
    PCI_IN_19_ENTRY,
    P009_ENTRY,
    PCI_LI_01_ENTRY,
    PCI_LI_02_ENTRY,
    PCI_LI_03_ENTRY,
    PCI_MM_01_ENTRY,
    PCI_MM_03_ENTRY,
    P047_ENTRY,
    P049_ENTRY,
    P095_ENTRY,
    P105_ENTRY,
    P039_ENTRY,
    P059_ENTRY,
    PCI_MSI_2_ENTRY,
    P042_ENTRY,
    E014_ENTRY,
    P019_ENTRY,
    PCI_PP_04_ENTRY,
    PCI_PP_05_ENTRY,
    P035_ENTRY,
    P046_ENTRY,
    P087_ENTRY,
    P015_ENTRY,
    P016_ENTRY,
    P083_ENTRY,
    P062_ENTRY,
    P069_ENTRY,
    E021_ENTRY,
    E008_ENTRY,
    P085_ENTRY,
    P084_ENTRY,
    P070_ENTRY,
    RE_REC_1_ENTRY,
    RE_REG_1_ENTRY,
    P056_ENTRY,
    P052_ENTRY,
    P063_ENTRY,
    RE_SMU_2_ENTRY,
    E036_ENTRY,
    P086_ENTRY,
    E022_ENTRY,
    E025_ENTRY,
    E026_ENTRY,
    E032_ENTRY,
    E030_ENTRY,
    P091_ENTRY,
    MPAM001_ENTRY,
    MPAM002_ENTRY,
    MPAM008_ENTRY,
    S_L7MP_03_ENTRY,
    MPAM009_ENTRY,
    MPAM003_ENTRY,
    MPAM005_ENTRY,
    MPAM006_ENTRY,
    MPAM007_ENTRY,
    ETE001_ENTRY,
    ETE002_ENTRY,
    ETE003_ENTRY,
    ETE004_ENTRY,
    ETE005_ENTRY,
    ETE006_ENTRY,
    ETE007_ENTRY,
    ETE008_ENTRY,
    ETE009_ENTRY,
    TPM001_ENTRY,
    TPM002_ENTRY,
    E007_ENTRY,
    E015_ENTRY,
    E016_ENTRY,
    E017_ENTRY,
    E034_ENTRY,
    P020_ENTRY,
    P022_ENTRY,
    P024_ENTRY,
    P025_ENTRY,
    P026_ENTRY,
    P033_ENTRY,
    P036_ENTRY,
    P045_ENTRY,
    P050_ENTRY,
    P060_ENTRY,
    P064_ENTRY,
    P068_ENTRY,
    P071_ENTRY,
    P103_ENTRY,
    E006_ENTRY,
    E019_ENTRY,
    E020_ENTRY,
    E033_ENTRY,
    E039_ENTRY,
    P028_ENTRY,
    P094_ENTRY,
    P096_ENTRY,
    P097_ENTRY,
    P104_ENTRY,
    TEST_ENTRY_SENTINEL
} TEST_ENTRY_ID_e;

/* Module ID enum definition */
typedef enum {
    BASE,
    PE,
    GIC,
    PERIPHERAL,
    MEM_MAP,
    PMU,
    RAS,
    SMMU,
    TIMER,
    WATCHDOG,
    NIST,
    PCIE,
    MPAM,
    ETE,
    TPM,
    POWER_WAKEUP,
    MODULE_ID_SENTINEL /* need to be in last */
} MODULE_NAME_e;

/* Flags to mark type of entry in rule_test_map_t */
typedef enum {
    INVALID_ENTRY = 0,
    BASE_RULE,
    ALIAS_RULE
} RULE_FLAG_e;

/* SBSA levels enum */
typedef enum {
    SBSA_LEVEL_3  = 3,
    SBSA_LEVEL_4  = 4,
    SBSA_LEVEL_5  = 5,
    SBSA_LEVEL_6  = 6,
    SBSA_LEVEL_7  = 7,
    SBSA_LEVEL_FR,
    SBSA_LEVEL_SENTINEL /* Keep last */
} SBSA_LEVEL_e;

/* BSA levels enum */
typedef enum {
    BSA_LEVEL_1 = 1,
    BSA_LEVEL_FR,
    BSA_LEVEL_SENTINEL /* Keep last */
} BSA_LEVEL_e;

/* PCBSA levels enum */
typedef enum {
    PCBSA_LEVEL_1 = 1,
    PCBSA_LEVEL_FR,
    PCBSA_LEVEL_SENTINEL /* Keep last */
} PCBSA_LEVEL_e;

/* Software views enum for BSA */
typedef enum {
    SW_OS,
    SW_HYP,
    SW_PS
} SOFTWARE_VIEW_e;

/* Platform enum definition */
typedef enum {
    PLATFORM_BAREMETAL = 1 << 0,  // 0x01
    PLATFORM_UEFI      = 1 << 1,  // 0x02
    PLATFORM_LINUX     = 1 << 2   // 0x04
} PLATFORM_e;

/* Architecture selection for rule expansion via -a */
typedef enum {
    ARCH_NONE = 0,
    ARCH_BSA,
    ARCH_SBSA,
    ARCH_PCBSA
} ARCH_SEL_e;

extern uint32_t g_arch_selection;

/* Level filter mode for CLI selection */
typedef enum {
    LVL_FILTER_NONE = 0,
    LVL_FILTER_MAX,   /* keep rules with level <= g_level_value */
    LVL_FILTER_ONLY,  /* keep rules with level == g_level_value */
    LVL_FILTER_FR     /* keep rules with level <= *_LEVEL_FR */
} LEVEL_FILTER_MODE_e;

/* ----------------------------  Struct  Definations --------------------------------------------*/

typedef uint32_t (*test_entry_fn_t)(uint32_t);
typedef uint32_t (*module_init_entry_t)(uint32_t);

/* Base rule id to test entry mapping structure definition*/
typedef struct {
    TEST_ENTRY_ID_e  test_entry_id;
    MODULE_NAME_e    module_id;     /* Module to which this test belongs */
    char8_t          rule_desc[RULE_DESC_SIZE];
    char8_t          platform_bitmask;
    RULE_FLAG_e      flag;
} rule_test_map_t;

/* Alias rules to Base rule mapping definition */
typedef struct {
    RULE_ID_e   alias_rule_id;
    RULE_ID_e   *base_rule_list;
    uint32_t    rule_count;
} alias_rule_map_t;

/* Module init status struct */
typedef struct {
    module_init_entry_t module_init;
    uint32_t            init_status;
} module_init_t;

/* rule list entry struct for rule_lookup.c */
typedef struct {
    RULE_ID_e rule_id;
    SBSA_LEVEL_e level;
} sbsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    BSA_LEVEL_e level;
    SOFTWARE_VIEW_e sw_view;
} bsa_rule_entry_t;

typedef struct {
    RULE_ID_e rule_id;
    PCBSA_LEVEL_e level;
} pcbsa_rule_entry_t;

/* ---------------------------- Helper functions declarations ---------------------------------- */
void     quick_sort_rule_list(RULE_ID_e *rule_list, uint32_t list_size);
uint32_t check_module_init(MODULE_NAME_e module_id);
uint32_t alias_rule_map_get_index(RULE_ID_e alias_rule_id);
void     print_rule_test_start(uint32_t rule_enum, uint32_t indent);
void     print_rule_test_status(uint32_t rule_enum, uint32_t status);
void     rule_status_map_reset(void);
bool     rule_in_list(RULE_ID_e rid, const RULE_ID_e *list, uint32_t count);

/* ---------------------------- Externs ---------------------------- */
extern uint32_t rule_status_map[RULE_ID_SENTINEL];
extern RULE_ID_e *g_skip_rule_list;
extern uint32_t   g_skip_rule_count;

/* Rule lookup tables (defined in rule_lookup.c) */
extern const bsa_rule_entry_t bsa_rule_list[];
extern const sbsa_rule_entry_t sbsa_rule_list[];
extern const pcbsa_rule_entry_t pcbsa_rule_list[];
/* Global selections configured by the app (unified_main.c) */
extern uint32_t g_level_filter_mode;  /* LEVEL_FILTER_MODE_e */
extern uint32_t g_level_value;        /* numeric value interpreted per-arch */
/* BSA-only software view selector bitmask; 0 means no filtering */
extern uint32_t g_bsa_sw_view_mask; /* bit (1<<SW_OS | 1<<SW_HYP | 1<<SW_PS) */

/* ------------------------------------ VAL APIs ------------------------------------------------*/
uint32_t filter_rule_list_by_cli(RULE_ID_e **rule_list, uint32_t list_size);
void run_tests(RULE_ID_e *rule_list, uint32_t list_size);

#endif /* __RULE_BASED_EXE_H__ */
