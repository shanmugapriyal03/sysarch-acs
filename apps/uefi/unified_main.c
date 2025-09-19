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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Protocol/ShellParameters.h>

#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/src/rule_based_execution.h"
#include "acs.h"


/* CLI parameter table for Unified ACS */
CONST SHELL_PARAM_ITEM ParamList[] = {
    {L"-v", TypeValue},    // -v    # Verbosity of the Prints. 1 shows all prints, 5 shows Errors
    {L"-f", TypeValue},    // -f    # Name of the log file to record the test results in.
    {L"-skip", TypeValue}, // -skip # test(s) to skip execution
    {L"-skip-dp-nic-ms", TypeFlag}, // Skip tests for DisplayPort, Network, and Mass Storage devices
    {L"-m", TypeValue},    // -m    # Module to be run
    {L"-p2p", TypeFlag},   // -p2p  # Peer-to-Peer is supported
    {L"-cache", TypeFlag}, // -cache# PCIe address translation cache is supported
    {L"-timeout", TypeValue}, // -timeout # Set timeout multiple for wakeup tests
    {L"-help", TypeFlag},  // -help # help : info about commands
    {L"-h", TypeFlag},     // -h    # help : info about commands
    {L"-dtb", TypeValue},  // -dtb  # Binary Flag to enable dtb dump
    {L"-a", TypeValue},    // -a    # Architecture selector: bsa | sbsa
    {L"-l", TypeValue},    // -l    # Max level to run (per-arch)
    {L"-only", TypeValue}, // -only # Run only the given level
    {L"-fr", TypeFlag},    // -fr   # Run rules up to the FR level (per-arch)
    {L"-hyp", TypeFlag},   // -hyp  # BSA software view filter: Hypervisor
    {L"-os", TypeFlag},    // -os   # BSA software view filter: OS
    {L"-ps", TypeFlag},    // -ps   # BSA software view filter: Platform Services
    {L"-no_crypto_ext", TypeFlag},  // -no_crypto_ext  # Skip tests which have export restrictions
    {L"-mmio", TypeFlag}, // -mmio # Enable pal_mmio prints
    {L"-el1physkip", TypeFlag}, // -el1physkip # Skips EL1 register checks
    {L"-r", TypeValue},        // -r    # Comma-separated Rule IDs for rule-based execution
    {L"-skipmodule", TypeValue}, // -skipmodule # Comma-separated module names to skip
    {L"-slc", TypeValue},    // -slc  # system last level cache type
    {NULL, TypeMax}
    };

VOID HelpMsg (VOID)
{
    Print (L"\nUsage: Bsa.efi [-v <n>] | [-f <filename>] | "
        "[-skip <n>] | [-m <n>]\n"
        "Options:\n"
        "-v      Verbosity of the prints\n"
        "        1 prints all, 5 prints only the errors\n"
        "        Note: pal_mmio prints can be enabled for specific modules by passing\n"
        "              module numbers along with global verbosity level 1\n"
        "              Module numbers are PE 0, MEM 1, GIC 2, SMMU 3, TIMER 4, WAKEUP 5   ...\n"
        "              PERIPHERAL 6, Watchdog 7, PCIe 8, Exerciser 9   ...\n"
        "              E.g., To enable mmio prints for PE and TIMER pass -v 104\n"
        "-mmio   Pass this flag to enable pal_mmio_read/write prints, use with -v 1\n"
        "-f      Name of the log file to record the test results in\n"
        "-skip   Rule ID(s) to be skipped (comma-separated, like -r)\n"
        "        Example: -skip B_PE_01,B_GIC_02\n"
        "-m      If Module ID(s) set, will only run the specified module(s), all others will be skipped.\n"
        "-no_crypto_ext  Pass this flag if cryptography extension not supported due to export restrictions\n"
        "-p2p    Pass this flag to indicate that PCIe Hierarchy Supports Peer-to-Peer\n"
        "-cache  Pass this flag to indicate that if the test system supports PCIe address translation cache\n"
        "-timeout  Set timeout multiple for wakeup tests\n"
        "        1 - min value  5 - max value\n"
        "-dtb    Enable the execution of dtb dump\n"
        "-a      Architecture selection: 'bsa', 'sbsa', or 'pcbsa'\n"
        "        -a bsa    Use full BSA rule checklist \n"
        "        -a sbsa   Use full SBSA rule checklist \n"
        "        -a pcbsa  Use full PC BSA rule checklist \n"
        "-l <n>  Max level to include (per-arch).\n"
        "        Defaults: bsa=1, sbsa=4, pcbsa=1.\n"
        "-only <n>  Include only rules at level <n> (per-arch).\n"
        "-fr     Run rules up to the FR level (per-arch).\n"
        "-hyp|-os|-ps  Software view filter (BSA only; can be combined).\n"
        "-el1physkip Skips EL1 register checks\n"
        "-slc    Provide system last level cache type\n"
        "        1 - PPTT PE-side cache,  2 - HMAT mem-side cache\n"
        "-skip-dp-nic-ms Skip PCIe tests for DisplayPort, Network, and Mass Storage devices\n"
        "-r      Rule selection: comma-separated IDs or a rules file\n"
        "        Examples: -r B_PE_01,B_PE_02,B_GIC_01\n"
        "                  -r rules.txt  (file may mix commas/newlines; lines starting with # are comments)\n"
    );
}

/* Apply default values to -a -l options if no value passed to cli */
static UINT32 apply_cli_defaults(VOID)
{
    if (g_arch_selection == ARCH_NONE && g_rule_count == 0) {
        Print(L"No -r or -a specified; defaulting to -a bsa\n");
        g_arch_selection = ARCH_BSA;
    }

    if (g_bsa_sw_view_mask != 0 && g_arch_selection != ARCH_BSA) {
        Print(L"-hyp/-os/-ps ignored: requires -a bsa.\n");
        g_bsa_sw_view_mask = 0;
    }

    if (g_level_filter_mode == LVL_FILTER_NONE && g_arch_selection != ARCH_NONE) {
        if (g_arch_selection == ARCH_BSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = BSA_LEVEL_1;
        } else if (g_arch_selection == ARCH_SBSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = SBSA_LEVEL_4;
        } else if (g_arch_selection == ARCH_PCBSA) {
            g_level_filter_mode = LVL_FILTER_MAX;
            g_level_value = PCBSA_LEVEL_1;
        }
    }

    return 0;
}

static VOID freeAcsMem(void)
{
    val_pe_free_info_table();
    val_gic_free_info_table();
    val_timer_free_info_table();
    val_wd_free_info_table();
    val_pcie_free_info_table();
    val_iovirt_free_info_table();
    val_peripheral_free_info_table();
    val_smbios_free_info_table();
    val_pmu_free_info_table();
    val_cache_free_info_table();
    val_mpam_free_info_table();
    val_hmat_free_info_table();
    val_srat_free_info_table();
    val_ras2_free_info_table();
    val_pcc_free_info_table();
    val_tpm2_free_info_table();
    val_free_shared_mem();
}

UINT32
execute_tests()
{
    VOID               *branch_label;
    UINT32             Status;

    /* Apply any ACS specific default values */
    Status = apply_cli_defaults();
    if (Status != 0) {
        if (g_dtb_log_file_handle) {
            ShellCloseFile(&g_dtb_log_file_handle);
        }
        if (g_acs_log_file_handle) {
            ShellCloseFile(&g_acs_log_file_handle);
        }
        return Status;
    }

    /* Print ACS header */
    val_print(ACS_PRINT_TEST, "\n\nSystem Architecture Compliance Suite", 0);
    val_print(ACS_PRINT_TEST, "\n          Version %d.", 1);
    val_print(ACS_PRINT_TEST, "%d.", 0);
    val_print(ACS_PRINT_TEST, "%d\n", 0);
    val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);
    val_print(ACS_PRINT_TEST, "\n       Creating Platform Information Tables\n", 0);


    /* Create info tables */
    Status = createPeInfoTable();
    if (Status) {
            if (g_acs_log_file_handle)
                ShellCloseFile(&g_acs_log_file_handle);
            return Status;
    }
    Status = createGicInfoTable();
    if (Status) {
            if (g_acs_log_file_handle)
                ShellCloseFile(&g_acs_log_file_handle);
            return Status;
    }

    /* Initialise exception vector, so any unexpected exception gets handled by default
    BSA exception handler */
    branch_label = &&exit_acs;
    val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
    val_pe_initialize_default_exception_handler(val_pe_default_esr);

    createTimerInfoTable();
    createWatchdogInfoTable();
    createPcieVirtInfoTable();
    createPeripheralInfoTable();
    createSmbiosInfoTable();
    createCacheInfoTable();
    createPccInfoTable();
    createMpamInfoTable();
    createHmatInfoTable();
    createSratInfoTable();
    createRas2InfoTable();
    createPmuInfoTable();
    createRasInfoTable();
    createTpm2InfoTable();
    val_allocate_shared_mem();
    FlushImage();

    /* Build and run rule-based selection if -r provided or -a selected (default BSA) */
    if ((g_rule_count > 0 && g_rule_list != NULL) || (g_arch_selection != ARCH_NONE)) {
        /* Merge arch rules if any, then apply CLI filters (-skip, -m, -skipmodule) */
        g_rule_count = filter_rule_list_by_cli(&g_rule_list, g_rule_count);
        if (g_rule_count == 0 || g_rule_list == NULL)
            goto exit_acs;

        /* Print rule selections */
        print_selection_summary();

        /* Run rule based test orchestrator */
        run_tests(g_rule_list, g_rule_count);
        return 0;
    }

exit_acs:

    freeAcsMem();

    /* Close any file handles open */
    if (g_dtb_log_file_handle) {
        ShellCloseFile(&g_dtb_log_file_handle);
    }
    if (g_acs_log_file_handle) {
        ShellCloseFile(&g_acs_log_file_handle);
    }

    val_pe_context_restore(AA64WriteSp(g_stack_pointer));
    return 0;
}
