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

/* @brief files contains test wrappers, which wraps multiple test entry functions required by
   single base rule into one single test entry function */

#include "include/acs_val.h"
#include "include/val_interface.h"
#include "include/acs_pcie.h"
#include "include/acs_gic.h"
#include "include/acs_wakeup.h"
#include "include/acs_mpam.h"
#include "include/acs_ras.h"
#include "include/acs_pe.h"
#include "include/acs_exerciser.h"
#include "include/test_wrappers.h"

/* B_PPI_00 */
uint32_t
b_ppi_00_entry(uint32_t num_pe)
{
    uint32_t status;

    status = g006_entry(num_pe);
    status |= g007_entry(num_pe);
    status |= g009_entry(num_pe);
    status |= g010_entry(num_pe);
    status |= g011_entry(num_pe);
    return status;
}

/* B_WAK_03, B_WAK_07 */
uint32_t
b_wak_03_07_entry(uint32_t num_pe)
{
    uint32_t status;

    status = u001_entry(num_pe);
    status |= u002_entry(num_pe);
    status |= u003_entry(num_pe);
    status |= u004_entry(num_pe);
    status |= u005_entry(num_pe);

    /* TODO: Test needs multi-PE interrupt handling support says comment in bsa_executes_tests.c*/
    /* status |= u006_entry(num_pe); */

    return status;
}

/* S_L7MP_03 */
uint32_t
s_l7mp_03_entry(uint32_t num_pe)
{
    uint32_t status;

    status = mpam002_entry(num_pe);
    status |= mpam006_entry(num_pe);
    status |= mpam007_entry(num_pe);
    return status;
}

/* SYS_RAS_2 */
uint32_t
sys_ras_2_entry(uint32_t num_pe)
{
    uint32_t status;

    status = ras011_entry(num_pe);
    status |= ras012_entry(num_pe);
    return status;
}

/* Appendix I.6 */
uint32_t
appendix_i_6_entry(uint32_t num_pe)
{
    uint32_t status;

    status = v2m001_entry(num_pe);
    status |= v2m003_entry(num_pe);
    return status;
}

/* P_L1PE_01 */
uint32_t
p_l1pe_01_entry(uint32_t num_pe)
{
    uint32_t status;

    status = pe004_entry(num_pe);
    status |= pe018_entry(num_pe);
    return status;
}

/* IE_REG_1 */
uint32_t
ie_reg_1_entry(uint32_t num_pe)
{
    uint32_t status;

    status = p049_entry(num_pe);
    status |= p059_entry(num_pe);
    return status;
}

/* PCI_IC_11 */
uint32_t
pci_ic_11_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p068_entry(num_pe);
    exr_status = e007_entry(num_pe);
    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_IN_04 */
uint32_t
pci_in_04_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p003_entry(num_pe);
    status |= p072_entry(num_pe);
    return status;
}

/* PCI_LI_02 */
uint32_t
pci_li_02_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    //TODO: revisit, need to deal with wrappers which has parts split across PALs
    //status = p096_entry(num_pe);
    status = ENCODE_STATUS(TEST_SKIP_VAL);

    exr_status = e006_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_LI_03 */
uint32_t
pci_li_03_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p023_entry(num_pe);
    status |= p078_entry(num_pe);
    return status;
}

/* PCI_MSI_2 */
uint32_t
pci_msi_2_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    //TODO: revisit, need to deal with wrappers which has parts split across PALs
    //status = p097_entry(num_pe);
    status = ENCODE_STATUS(TEST_SKIP_VAL);

    exr_status = e033_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_PP_04 */
uint32_t
pci_pp_04_entry(uint32_t num_pe)
{
    uint32_t exr_status;

    exr_status = e001_entry(num_pe);
    exr_status |= e002_entry(num_pe);

    return exr_status;
}

/* PCI_PP_05 */
uint32_t
pci_pp_05_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p017_entry(num_pe);
    status |= p018_entry(num_pe);
    return status;
}

/* RE_REC_1 */
uint32_t
re_rec_1_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p053_entry(num_pe);
    status |= p061_entry(num_pe);
    return status;
}

/* RE_REG_1 */
uint32_t
re_reg_1_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p048_entry(num_pe);
    status |= p058_entry(num_pe);
    return status;
}

/* IE_REG_3 */
uint32_t
ie_reg_3_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p050_entry(num_pe);
    status |= p060_entry(num_pe);
    exr_status = e034_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_IN_19 */
uint32_t
pci_in_19_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p030_entry(num_pe);
    status |= p031_entry(num_pe);
    status |= p032_entry(num_pe);
    return status;
}

/* PCI_LI_01 */
uint32_t
pci_li_01_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p006_entry(num_pe);
    status |= p027_entry(num_pe);
    return status;
}

/* PCI_MM_01 */
uint32_t
pci_mm_01_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p045_entry(num_pe);
    status |= p103_entry(num_pe);
    exr_status = e016_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_MM_03 */
uint32_t
pci_mm_03_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    //TODO: revisit, need to deal with wrappers which has parts split across PALs
    //status = p094_entry(num_pe);
    //status = p0104_entry(num_pe);
    status = ENCODE_STATUS(TEST_SKIP_VAL);

    exr_status = e039_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* RE_SMU_2 */
uint32_t
re_smu_2_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p028_entry(num_pe);
    exr_status = e019_entry(num_pe);
    exr_status |= e020_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* IE_REG_2 */
uint32_t
ie_reg_2_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p054_entry(num_pe);
    status |= p098_entry(num_pe);
    status |= p065_entry(num_pe);
    status |= p067_entry(num_pe);
    return status;
}

/* IE_REG_4 */
uint32_t
ie_reg_4_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p051_entry(num_pe);
    status |= p099_entry(num_pe);
    status |= p066_entry(num_pe);
    status |= p088_entry(num_pe);
    return status;
}

/* PCI_IN_13 */
uint32_t
pci_in_13_entry(uint32_t num_pe)
{
    uint32_t status;
    status = p004_entry(num_pe);
    status |= p005_entry(num_pe);
    return status;
}

/* PCI_IN_17 */
uint32_t
pci_in_17_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p036_entry(num_pe);
    status |= p064_entry(num_pe);
    status |= p071_entry(num_pe);
    exr_status = e015_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}

/* PCI_IN_05 */
uint32_t
pci_in_05_entry(uint32_t num_pe)
{
    uint32_t status;
    uint32_t exr_status;

    status = p020_entry(num_pe);
    status |= p022_entry(num_pe);
    status |= p024_entry(num_pe);
    status |= p025_entry(num_pe);
    status |= p026_entry(num_pe);
    status |= p033_entry(num_pe);
    exr_status = e017_entry(num_pe);

    /* If PCIe static checks passed, but exerciser part skipped maybe due to exerciser not present
       report rule as partially covered */
    if (IS_TEST_SKIP(exr_status) && IS_TEST_PASS(status)) {
        return ENCODE_STATUS(TEST_PARTIAL_COV);
    }

    return status | exr_status;
}
