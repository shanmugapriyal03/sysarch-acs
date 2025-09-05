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

extern VOID* g_acs_log_file_handle;
extern VOID* g_dtb_log_file_handle;
extern  UINT32 g_sw_view[3];
/* Use rule string map from VAL to translate -r inputs */
extern char8_t *rule_id_string[RULE_ID_SENTINEL];
/* Use module string map from VAL to translate -m inputs */
extern char8_t *module_name_string[MODULE_ID_SENTINEL];

UINT32  g_pcbsa_level;
UINT32  g_pcbsa_only_level = 0;
UINT32  g_pcie_p2p;
UINT32  g_pcie_cache_present;
bool    g_pcie_skip_dp_nic_ms = 0;
UINT32  g_bsa_level;
UINT32  g_bsa_only_level = 0;
UINT32  g_sbsa_level;
UINT32  g_sbsa_only_level = 0;
UINT32  g_print_level;
UINT32  g_sw_view[3] = {1, 1, 1}; //Operating System, Hypervisor, Platform Security
UINT32  *g_skip_test_num;
UINT32  g_num_skip;
UINT32  g_acs_tests_total;
UINT32  g_acs_tests_pass;
UINT32  g_acs_tests_fail;
UINT64  g_stack_pointer;
UINT64  g_exception_ret_addr;
UINT64  g_ret_addr;
UINT32  g_wakeup_timeout;
UINT32  g_build_sbsa = 0;
UINT32  g_build_pcbsa = 0;
UINT32  g_print_mmio;
UINT32  g_curr_module;
UINT32  g_enable_module;
UINT32  *g_execute_tests;
UINT32  g_crypto_support = TRUE;
UINT32  g_num_tests = 0;
UINT32  *g_execute_modules;
UINT32  g_num_modules = 0;
UINT32  *g_skip_modules;
UINT32  g_num_skip_modules = 0;
UINT32  g_sys_last_lvl_cache;
/* VE systems run acs at EL1 and in some systems crash is observed during acess
   of EL1 phy and virt timer, Below command line option is added only for debug
   purpose to complete BSA run on these systems */
UINT32  g_el1physkip = FALSE;

SHELL_FILE_HANDLE g_acs_log_file_handle;
SHELL_FILE_HANDLE g_dtb_log_file_handle;

/* Storage for parsed rule IDs from -r */
static RULE_ID_e *g_rule_list;
static UINT32     g_rule_count;
/* Storage for parsed skip rule IDs from -skip (global for cross-file access) */
RULE_ID_e *g_skip_rule_list = NULL;
UINT32     g_skip_rule_count = 0;

/* Minimal ASCII string equality check to avoid extra deps */
static BOOLEAN ascii_streq(const CHAR8 *a, const CHAR8 *b)
{
  if (a == NULL || b == NULL) return FALSE;
  while (*a && *b) {
    if (*a != *b) return FALSE;
    a++; b++;
  }
  return (*a == '\0' && *b == '\0');
}

/* Case-insensitive ASCII compare for CHAR16 literals (only A-Z handled) */
static BOOLEAN w_ascii_streq_caseins(const CHAR16 *a, const CHAR16 *b)
{
  if (a == NULL || b == NULL) return FALSE;
  while (*a && *b) {
    CHAR16 ca = *a;
    CHAR16 cb = *b;
    if (ca >= L'A' && ca <= L'Z') ca = (CHAR16)(ca - L'A' + L'a');
    if (cb >= L'A' && cb <= L'Z') cb = (CHAR16)(cb - L'A' + L'a');
    if (ca != cb) return FALSE;
    a++; b++;
  }
  return (*a == L'\0' && *b == L'\0');
}

/* Check if a rule already exists in a list (first count entries) */
static BOOLEAN rule_in_list(RULE_ID_e rid, const RULE_ID_e *list, UINT32 count)
{
  for (UINT32 i = 0; i < count; i++) {
    if (list[i] == rid) return TRUE;
  }
  return FALSE;
}

/* Map a wide token to MODULE_NAME_e and append to list if capacity allows.
   Accepts only module names that match module_name_string[] (no numeric IDs). */
static VOID map_and_add_module_token(
  const CHAR16 *wtoken,
  UINT32 *list,
  UINT32 *count,
  UINT32 capacity
)
{
  if (wtoken == NULL) return;
  /* trim leading/trailing whitespace including CR/LF */
  const CHAR16 *start = wtoken;
  while (*start == L' ' || *start == L'\t' || *start == L'\r' || *start == L'\n') start++;
  const CHAR16 *end = start;
  while (*end)
    end++;
  while (end > start &&
         (end[-1] == L' ' || end[-1] == L'\t' || end[-1] == L'\r' || end[-1] == L'\n'))
    end--;
  if (end <= start) return;

  /* Copy token into bounded buffer */
  CHAR16 wbuf[32];
  UINTN tlen = (UINTN)(end - start);
  if (tlen >= (sizeof(wbuf)/sizeof(wbuf[0]))) tlen = (sizeof(wbuf)/sizeof(wbuf[0])) - 1;
  for (UINTN i = 0; i < tlen; i++) wbuf[i] = start[i];
  wbuf[tlen] = L'\0';

  /* Convert to ASCII and compare to module_name_string */
  CHAR8 abuf[32];
  UINTN i = 0;
  for (; i < tlen && i < (sizeof(abuf) - 1); i++)
    abuf[i] = (CHAR8)(wbuf[i] & 0xFF);
  abuf[i] = '\0';

  for (UINT32 mid = 0; mid < MODULE_ID_SENTINEL; mid++) {
    if (module_name_string[mid] &&
        ascii_streq((CHAR8 *)module_name_string[mid], abuf)) {
      if (*count < capacity) list[(*count)++] = mid;
      return;
    }
  }

  Print(L"Invalid module: %s\n", wbuf);
}

/* Map a wide token to RULE_ID_e and append to list if capacity allows */
static VOID map_and_add_rule_token(
  const CHAR16 *wtoken,
  RULE_ID_e *list,
  UINT32 *count,
  UINT32 capacity
)
{
  /* trim leading/trailing spaces and tabs */
  if (wtoken == NULL) return;
  const CHAR16 *start = wtoken;
  while (*start == L' ' || *start == L'\t') start++;
  const CHAR16 *end = start;
  while (*end)
    end++;
  while (end > start && (end[-1] == L' ' || end[-1] == L'\t'))
    end--;
  if (end <= start) return; /* empty token */

  /* Copy into bounded buffer */
  CHAR16 wbuf[64];
  UINTN tlen = (UINTN)(end - start);
  if (tlen >= (sizeof(wbuf)/sizeof(wbuf[0]))) tlen = (sizeof(wbuf)/sizeof(wbuf[0])) - 1;
  for (UINTN i = 0; i < tlen; i++) wbuf[i] = start[i];
  wbuf[tlen] = L'\0';

  /* Convert to ASCII for comparison with rule_id_string */
  CHAR8 abuf[64];
  UINTN i = 0;
  for (; i < tlen && i < (sizeof(abuf) - 1); i++)
    abuf[i] = (CHAR8)(wbuf[i] & 0xFF);
  abuf[i] = '\0';

  for (UINT32 rid = 0; rid < RULE_ID_SENTINEL; rid++) {
    if (rule_id_string[rid] &&
        ascii_streq((CHAR8 *)rule_id_string[rid], abuf)) {
      if (*count < capacity) {
        list[(*count)++] = (RULE_ID_e)rid;
      }
      return;
    }
  }

  /* Not found: print invalid once for visibility */
  Print(L"Invalid rule id: %s\n", wbuf);
}

/* Try open a file read-only; return TRUE if opened */
static BOOLEAN try_open_readonly(const CHAR16 *Path, SHELL_FILE_HANDLE *Handle)
{
  EFI_STATUS S = ShellOpenFileByName(Path, Handle, EFI_FILE_MODE_READ, 0);
  return (S == EFI_SUCCESS && *Handle != NULL);
}

/* Read entire file into a byte buffer (dynamically grown) */
static EFI_STATUS read_all_bytes(SHELL_FILE_HANDLE Handle, UINT8 **OutBuf, UINTN *OutLen)
{
  const UINTN CHUNK = 1024;
  UINTN cap = 2048;
  UINT8 *buf = NULL;
  EFI_STATUS Status = gBS->AllocatePool(EfiBootServicesData, cap, (VOID **)&buf);

  if (EFI_ERROR(Status))
    return Status;
  UINTN len = 0;

  /* Reset to start */
  ShellSetFilePosition(Handle, 0);

  for (;;) {
    UINT8 tmp[CHUNK];
    UINTN want = sizeof(tmp);
    EFI_STATUS S = ShellReadFile(Handle, &want, tmp);

    if (EFI_ERROR(S)) {
      Status = S;
      break;
    }
    if (want == 0) break; /* EOF */
    /* grow if needed */
    if (len + want > cap) {
      UINTN ncap = (cap * 3) / 2;
      if (ncap < len + want) ncap = len + want + 512;
      UINT8 *nbuf;
      Status = gBS->AllocatePool(EfiBootServicesData, ncap, (VOID **)&nbuf);
      if (EFI_ERROR(Status))
        break;
      for (UINTN i = 0; i < len; i++) nbuf[i] = buf[i];
      gBS->FreePool(buf);
      buf = nbuf; cap = ncap;
    }
    for (UINTN j = 0; j < want; j++) buf[len + j] = tmp[j];
    len += want;
  }

  if (EFI_ERROR(Status)) {
    if (buf) gBS->FreePool(buf);
    return Status;
  }

  *OutBuf = buf; *OutLen = len;
  return EFI_SUCCESS;
}

/* Convert raw bytes to a temporary CHAR16 buffer, handling ASCII or UTF-16LE */
static EFI_STATUS bytes_to_wchars(const UINT8 *bytes, UINTN blen, CHAR16 **out, UINTN *wlen)
{
  if (bytes == NULL || blen == 0) { *out = NULL; *wlen = 0; return EFI_SUCCESS; }

  BOOLEAN utf16 = FALSE; UINTN offset = 0;
  if (blen >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) { utf16 = TRUE; offset = 2; }
  else {
    /* heuristic: many zeros suggests UTF-16LE */
    UINTN zeros = 0;
    for (UINTN i = 0; i < blen; i++) if (bytes[i] == 0) zeros++;
    if (zeros > (blen / 4)) utf16 = TRUE;
  }

  if (utf16) {
    UINTN c = (blen - offset) / 2;
    EFI_STATUS S = gBS->AllocatePool(EfiBootServicesData,
                                     (c + 1) * sizeof(CHAR16),
                                     (VOID **)out);

    if (EFI_ERROR(S))
      return S;
    const UINT8 *p = bytes + offset;
    for (UINTN i = 0; i < c; i++) {
      (*out)[i] = (CHAR16)(p[2*i] | ((UINT16)p[2*i + 1] << 8));
    }
    (*out)[c] = L'\0'; *wlen = c;
  } else {
    EFI_STATUS S = gBS->AllocatePool(EfiBootServicesData,
                                     (blen + 1) * sizeof(CHAR16),
                                     (VOID **)out);

    if (EFI_ERROR(S))
      return S;
    for (UINTN i = 0; i < blen; i++) (*out)[i] = (CHAR16)bytes[i];
    (*out)[blen] = L'\0'; *wlen = blen;
  }
  return EFI_SUCCESS;
}

/* Count tokens in a rules text (delims: comma/newline; lines starting with # ignored) */
static UINT32 count_rule_tokens_in_text(const CHAR16 *text, UINTN len)
{
  UINT32 count = 0; BOOLEAN in_token = FALSE; BOOLEAN line_start = TRUE; BOOLEAN in_comment = FALSE;
  for (UINTN i = 0; i < len; i++) {
    CHAR16 ch = text[i];
    if (line_start) {
      /* skip leading spaces/tabs at line start */
      while ((ch == L' ' || ch == L'\t') && (i + 1) < len) ch = text[++i];
      if (ch == L'#') in_comment = TRUE;
      line_start = FALSE;
    }
    if (in_comment) {
      if (ch == L'\n' || ch == L'\r') { in_comment = FALSE; line_start = TRUE; }
      continue;
    }
    if (ch == L',' || ch == L'\n' || ch == L'\r') {
      if (in_token) { count++; in_token = FALSE; }
      if (ch == L'\n' || ch == L'\r') { line_start = TRUE; }
    } else if (ch != L' ' && ch != L'\t') {
      in_token = TRUE;
    }
    if (ch == L'\n' || ch == L'\r') { line_start = TRUE; }
  }
  if (in_token) count++;
  return count;
}

/* Parse a rules text buffer into list using same tokenization */
static VOID
parse_rules_text_into_list(
  const CHAR16 *text,
  UINTN len,
  RULE_ID_e *list,
  UINT32 *count,
  UINT32 capacity)
{
  UINTN i = 0; BOOLEAN line_start = TRUE; BOOLEAN in_comment = FALSE;
  while (i < len) {
    /* handle line-start comment */
    if (line_start) {
      /* skip leading spaces/tabs */
      while (i < len && (text[i] == L' ' || text[i] == L'\t')) i++;
      if (i < len && text[i] == L'#') {
        in_comment = TRUE;
      }
      line_start = FALSE;
    }

    if (in_comment) {
      while (i < len && text[i] != L'\n' && text[i] != L'\r')
        i++;
      if (i < len && (text[i] == L'\n' || text[i] == L'\r')) {
        line_start = TRUE;
        in_comment = FALSE;
        i++;
      }
      continue;
    }

    /* capture token until delimiter */
    UINTN start = i;
    while (i < len && text[i] != L',' && text[i] != L'\n' && text[i] != L'\r')
      i++;
    UINTN end = i;

    /* trim */
    while (start < end && (text[start] == L' ' || text[start] == L'\t')) start++;
    while (end > start && (text[end-1] == L' ' || text[end-1] == L'\t'))
      end--;
    if (end > start) {
      CHAR16 wtmp[64];
      UINTN tlen = end - start;
      if (tlen >= (sizeof(wtmp)/sizeof(wtmp[0])))
        tlen = (sizeof(wtmp)/sizeof(wtmp[0])) - 1;
      for (UINTN k = 0; k < tlen; k++) wtmp[k] = text[start + k];
      wtmp[tlen] = L'\0';
      map_and_add_rule_token(wtmp, list, count, capacity);
    }

    if (i < len && (text[i] == L',' || text[i] == L'\n' || text[i] == L'\r')) {
      if (text[i] == L'\n' || text[i] == L'\r') line_start = TRUE;
      i++;
    }
  }
}

VOID
HelpMsg (
  VOID
  )
{
  Print (L"\nUsage: Bsa.efi [-v <n>] | [-l <n>] | [-only] | [-fr] | [-f <filename>] | "
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
         "-l      Level of compliance to be tested for\n"
         "        As per BSA specification, Valid level is 1\n"
         "-only   To only run tests belonging to a specific level of compliance\n"
         "        -l (level) or -fr option needs to be specified for using this flag\n"
         "-fr     Should be passed without level option to run future requirement tests\n"
         "-f      Name of the log file to record the test results in\n"
         "-skip   Rule ID(s) to be skipped (comma-separated, like -r)\n"
         "        Example: -skip B_PE_01,B_GIC_02\n"
         "-m      If Module ID(s) set, will only run the specified module(s), all others will be skipped.\n"
         "-no_crypto_ext  Pass this flag if cryptography extension not supported due to export restrictions\n"
         "-p2p    Pass this flag to indicate that PCIe Hierarchy Supports Peer-to-Peer\n"
         "-cache  Pass this flag to indicate that if the test system supports PCIe address translation cache\n"
         "-timeout  Set timeout multiple for wakeup tests\n"
         "        1 - min value  5 - max value\n"
         "-os     Enable the execution of operating system tests\n"
         "-hyp    Enable the execution of hypervisor tests\n"
         "-ps     Enable the execution of platform security tests\n"
         "-dtb    Enable the execution of dtb dump\n"
         "-a      Architecture selection: 'bsa', 'sbsa', or 'pcbsa'\n"
         "-pc-bsa Enable PC BSA requirements for bsa binary\n"
         "-el1physkip Skips EL1 register checks\n"
         "-skip-dp-nic-ms Skip PCIe tests for DisplayPort, Network, and Mass Storage devices\n"
         "-r      Rule selection: comma-separated IDs or a rules file\n"
         "        Examples: -r B_PE_01,B_PE_02,B_GIC_01\n"
         "                  -r rules.txt  (file may mix commas/newlines; lines starting with # are comments)\n"
         "        -a bsa    Use full BSA rule checklist (ignores -r)\n"
         "        -a sbsa   Use full SBSA rule checklist (ignores -r)\n"
         "        -a pcbsa  Use full PC BSA rule checklist (ignores -r)\n"
  );
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeValue},    // -v    # Verbosity of the Prints. 1 shows all prints, 5 shows Errors
  {L"-l", TypeValue},    // -l    # Level of compliance to be tested for.
  {L"-only", TypeValue},    // -only # To only run tests for a Specific level of compliance.
  {L"-fr", TypeValue},    // -fr   # To run BSA ACS till BSA Future Requirement tests
  {L"-f", TypeValue},    // -f    # Name of the log file to record the test results in.
  {L"-skip", TypeValue}, // -skip # test(s) to skip execution
  {L"-skip-dp-nic-ms", TypeFlag}, // Skip tests for DisplayPort, Network, and Mass Storage devices
  {L"-m", TypeValue},    // -m    # Module to be run
  {L"-p2p", TypeFlag},   // -p2p  # Peer-to-Peer is supported
  {L"-cache", TypeFlag}, // -cache# PCIe address translation cache is supported
  {L"-timeout", TypeValue}, // -timeout # Set timeout multiple for wakeup tests
  {L"-help", TypeFlag},  // -help # help : info about commands
  {L"-h", TypeFlag},     // -h    # help : info about commands
  {L"-os", TypeFlag},    // -os   # Binary Flag to enable the execution of operating system tests.
  {L"-hyp", TypeFlag},   // -hyp  # Binary Flag to enable the execution of hypervisor tests.
  {L"-ps", TypeFlag},    // -ps   # Binary Flag to enable the execution of platform security tests.
  {L"-dtb", TypeValue},  // -dtb  # Binary Flag to enable dtb dump
  {L"-a", TypeValue},    // -a    # Architecture selector: bsa | sbsa
  {L"-pc-bsa", TypeFlag},  // -pc-bsa # Enable PC SBA requirements for bsa binary\n"
  {L"-no_crypto_ext", TypeFlag},  // -no_crypto_ext  # Skip tests which have export restrictions
  {L"-mmio", TypeFlag}, // -mmio # Enable pal_mmio prints
  {L"-el1physkip", TypeFlag}, // -el1physkip # Skips EL1 register checks
  {L"-r", TypeValue},        // -r    # Comma-separated Rule IDs for rule-based execution
  {L"-skipmodule", TypeValue}, // -skipmodule # Comma-separated module names to skip
  {NULL, TypeMax}
  };

/***
  BSA Compliance Suite Entry Point.

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
UINT32
command_init ()
{

  LIST_ENTRY         *ParamPackage;
  CONST CHAR16       *CmdLineArg;
  CHAR16             *ProbParam;
  UINT32             Status;
  UINT32             ReadVerbosity;

  //
  // Process Command Line arguments
  //
  Status = ShellInitialize();
  Status = ShellCommandLineParse (ParamList, &ParamPackage, &ProbParam, TRUE);
  if (Status) {
    Print(L"Shell command line parse error %x\n", Status);
    Print(L"Unrecognized option %s passed\n", ProbParam);
    HelpMsg();
    return SHELL_INVALID_PARAMETER;
  }

  /* Validate -a argument if provided */
  CONST CHAR16 *ArchArg = ShellCommandLineGetValue(ParamPackage, L"-a");
  if (ArchArg != NULL) {
    if (!(w_ascii_streq_caseins(ArchArg, L"bsa") ||
          w_ascii_streq_caseins(ArchArg, L"sbsa") ||
          w_ascii_streq_caseins(ArchArg, L"pcbsa"))) {
      Print(L"Invalid value for -a. Use 'bsa', 'sbsa', or 'pcbsa'\n");
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
    }
  }
  if (ShellCommandLineGetFlag (ParamPackage, L"-skip")) {
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skip");
    if (CmdLineArg == NULL)
    {
      Print(L"Invalid parameter passed for -skip\n", 0);
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
    }
    else
    {
      /* Count tokens */
      UINTN len = StrLen(CmdLineArg);
      UINT32 max_tokens = 1; UINTN k;
      for (k = 0; k < len; k++) if (CmdLineArg[k] == L',') max_tokens++;

      if (max_tokens > 0) {
        EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                  max_tokens * sizeof(RULE_ID_e),
                                  (VOID **)&g_skip_rule_list);
        if (EFI_ERROR(EStatus))
        {
          Print(L"Allocate memory for -skip failed\n", 0);
          return 0;
        }
      }

      g_skip_rule_count = 0;
      UINTN start = 0;
      while (start < len) {
        UINTN end = start;
        while (end < len && CmdLineArg[end] != L',')
          end++;
        /* trim spaces/tabs */
        while (start < end && (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t'))
          start++;
        while (end > start && (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t'))
          end--;
        if (end > start) {
          CHAR16 wtoken[64];
          UINTN tlen = end - start;
          if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
            tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
          for (UINTN ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
          wtoken[tlen] = L'\0';
          map_and_add_rule_token(wtoken, g_skip_rule_list, &g_skip_rule_count, max_tokens);
        }
        start = (end < len) ? end + 1 : end;
      }
    }
  }

  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-timeout");
  if (CmdLineArg == NULL) {
    g_wakeup_timeout = 1;
  } else {
    g_wakeup_timeout = StrDecimalToUintn(CmdLineArg);
    Print(L"Wakeup timeout multiple %d.\n", g_wakeup_timeout);
    if (g_wakeup_timeout > 5)
        g_wakeup_timeout = 5;
    }

    // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-v");
  if (CmdLineArg == NULL) {
    g_print_level = G_PRINT_LEVEL;
  } else {
    ReadVerbosity = StrDecimalToUintn(CmdLineArg);
    while (ReadVerbosity/10) {
      g_enable_module |= (1 << ReadVerbosity%10);
      ReadVerbosity /= 10;
    }
    g_print_level = ReadVerbosity;
    if (g_print_level > 5) {
      g_print_level = G_PRINT_LEVEL;
    }
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-mmio")) {
    g_print_mmio = TRUE;
  } else {
    g_print_mmio = FALSE;
  }

  g_bsa_level = G_BSA_LEVEL;
  g_sbsa_level = G_SBSA_LEVEL;
  g_pcbsa_level = G_PCBSA_LEVEL;


  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-l");
  if (CmdLineArg == NULL) {

    if ((ShellCommandLineGetFlag (ParamPackage, L"-only")) &&
         !(ShellCommandLineGetFlag (ParamPackage, L"-fr"))) {
         Print(L" Only option(-only) have to be passed along with Level option (-l)\n", 0);
         HelpMsg();
         return SHELL_INVALID_PARAMETER;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-fr")) {
      if (ArchArg && w_ascii_streq_caseins(ArchArg, L"sbsa")) {
        g_bsa_level = BSA_MAX_LEVEL_SUPPORTED + 1;
        g_sbsa_level = SBSA_MAX_LEVEL_SUPPORTED + 1;
        if (ShellCommandLineGetFlag (ParamPackage, L"-only"))
        {
          g_bsa_only_level = g_bsa_level;
          g_sbsa_only_level = g_sbsa_level;
        }
      }
      else if (ShellCommandLineGetFlag (ParamPackage, L"-pc-bsa")) {
        g_bsa_level = BSA_MAX_LEVEL_SUPPORTED + 1;
        g_pcbsa_level = PCBSA_MAX_LEVEL_SUPPORTED + 1;
        if (ShellCommandLineGetFlag (ParamPackage, L"-only"))
        {
          g_bsa_only_level = g_bsa_level;
          g_pcbsa_only_level = g_pcbsa_level;
        }
      }
      else {
      g_bsa_level = BSA_MAX_LEVEL_SUPPORTED + 1;
      g_sbsa_level = SBSA_MAX_LEVEL_SUPPORTED + 1;
      if (ShellCommandLineGetFlag (ParamPackage, L"-only"))
        g_bsa_only_level = g_bsa_level;
      }
    }
  } else {
    if (ArchArg && w_ascii_streq_caseins(ArchArg, L"sbsa")) {
        g_sbsa_level = StrDecimalToUintn(CmdLineArg);
        if (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) {
          Print(L"SBSA Level %d is not supported.\n", g_sbsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (g_sbsa_level < SBSA_MIN_LEVEL_SUPPORTED) {
          Print(L"SBSA Level %d is not supported.\n", g_sbsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (ShellCommandLineGetFlag (ParamPackage, L"-only")) {
            g_sbsa_only_level = g_sbsa_level;
        }
    }
    else if (ShellCommandLineGetFlag (ParamPackage, L"-pc-bsa")) {
        g_pcbsa_level = StrDecimalToUintn(CmdLineArg);
        if (g_pcbsa_level > PCBSA_MAX_LEVEL_SUPPORTED) {
          Print(L"PCBSA Level %d is not supported.\n", g_pcbsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (g_pcbsa_level < PCBSA_MIN_LEVEL_SUPPORTED) {
          Print(L"PCBSA Level %d is not supported.\n", g_pcbsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (ShellCommandLineGetFlag (ParamPackage, L"-only")) {
            g_pcbsa_only_level = g_pcbsa_level;
        }
    }
    else {
        g_bsa_level = StrDecimalToUintn(CmdLineArg);
        if (g_bsa_level > BSA_MAX_LEVEL_SUPPORTED) {
          Print(L"BSA Level %d is not supported.\n", g_bsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (g_bsa_level < BSA_MIN_LEVEL_SUPPORTED) {
          Print(L"BSA Level %d is not supported.\n", g_bsa_level);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
        }
        if (ShellCommandLineGetFlag (ParamPackage, L"-only")) {
            g_bsa_only_level = g_bsa_level;
        }
    }
  }

  // Options with Flags
   if (ShellCommandLineGetFlag (ParamPackage, L"-os")
       || ShellCommandLineGetFlag (ParamPackage, L"-hyp")
       || ShellCommandLineGetFlag (ParamPackage, L"-ps")) {
       g_sw_view[G_SW_OS]  = 0;
       g_sw_view[G_SW_HYP] = 0;
       g_sw_view[G_SW_PS]  = 0;

       if (ShellCommandLineGetFlag (ParamPackage, L"-os"))
           g_sw_view[G_SW_OS] = 1;

       if (ShellCommandLineGetFlag (ParamPackage, L"-hyp"))
           g_sw_view[G_SW_HYP] = 1;

       if (ShellCommandLineGetFlag (ParamPackage, L"-ps"))
           g_sw_view[G_SW_PS] = 1;
  }


    // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-f");
  if (CmdLineArg == NULL) {
    g_acs_log_file_handle = NULL;
  } else {
    Status = ShellOpenFileByName(CmdLineArg, &g_acs_log_file_handle,
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
    if(EFI_ERROR(Status)) {
         Print(L"Failed to open log file %s\n", CmdLineArg);
         g_acs_log_file_handle = NULL;
    }
  }

  /* get System Last-level cache info */
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-slc");
  if (CmdLineArg == NULL) {
    g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
  } else {
    g_sys_last_lvl_cache = StrDecimalToUintn(CmdLineArg);
    if (g_sys_last_lvl_cache > 2 || g_sys_last_lvl_cache < 1) {
        Print(L"Invalid value provided for -slc, Value = %d\n", g_sys_last_lvl_cache);
        g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
    }
  }

    // If user has pass dtb flag, then dump the dtb in file
  CmdLineArg  = ShellCommandLineGetValue(ParamPackage, L"-dtb");
  if (CmdLineArg == NULL) {
    g_dtb_log_file_handle = NULL;
  } else {
    Status = ShellOpenFileByName(CmdLineArg, &g_dtb_log_file_handle,
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
    if (EFI_ERROR(Status)) {
         Print(L"Failed to open file for dtb dump %s\n", CmdLineArg);
         g_dtb_log_file_handle = NULL;
    } else {
        val_dump_dtb();
    }
  }

  // Options with Flags
  if ((ShellCommandLineGetFlag (ParamPackage, L"-help")) || (ShellCommandLineGetFlag (ParamPackage, L"-h"))){
     HelpMsg();
     return 0;
  }

  // Options with Flags
  if ((ShellCommandLineGetFlag (ParamPackage, L"-no_crypto_ext")))
     g_crypto_support = FALSE;


  /* Options with Values: -r <comma-separated rule IDs or rules file> */
  if (ShellCommandLineGetFlag(ParamPackage, L"-r")) {
      CmdLineArg = ShellCommandLineGetValue(ParamPackage, L"-r");
      if (CmdLineArg == NULL) {
          Print(L"Invalid parameter passed for -r\n");
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
      }

      /* If the -r list was split by spaces, collect subsequent args until next option */
      CHAR16 *Combined = NULL;
      UINTN CombinedLen = 0;
      EFI_SHELL_PARAMETERS_PROTOCOL *ShellParams = NULL;

      if (!EFI_ERROR(gBS->OpenProtocol(
              gImageHandle,
              &gEfiShellParametersProtocolGuid,
              (VOID **)&ShellParams,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL))) {
          /* Walk argv to find the "-r" and concatenate following non-option args */
          for (UINTN ai = 0; ai + 1 < ShellParams->Argc; ai++) {
              if (StrCmp(ShellParams->Argv[ai], L"-r") == 0) {
                  /* Start with existing value from GetValue */
                  UINTN baseLen = StrLen(CmdLineArg);
                  /* Compute total extra length of trailing segments */
                  UINTN extra = 0;
                  for (UINTN j = ai + 2; j < ShellParams->Argc; j++) {
                      if (ShellParams->Argv[j][0] == L'-')
                          break;
                      /* +1 for possible inserted comma */
                      extra += StrLen(ShellParams->Argv[j]) + 1;
                  }
                  if (extra > 0) {
                      UINTN alloc = baseLen + extra + 1;

                      if (!EFI_ERROR(gBS->AllocatePool(EfiBootServicesData,
                                                      alloc * sizeof(CHAR16),
                                                      (VOID **)&Combined))) {

                          /* Copy base */
                          for (UINTN t = 0; t < baseLen; t++) Combined[t] = CmdLineArg[t];
                          CombinedLen = baseLen; Combined[CombinedLen] = L'\0';
                          /* Append segments with delimiter if needed */
                          for (UINTN j = ai + 2; j < ShellParams->Argc; j++) {
                              CHAR16 *seg = ShellParams->Argv[j];
                              if (seg[0] == L'-') break;
                              /* ensure previous ends with a delimiter */
                              if (CombinedLen > 0) {
                                  CHAR16 last = Combined[CombinedLen - 1];
                                  if (last != L',' && last != L'\n' && last != L'\r') {
                                      Combined[CombinedLen++] = L',';
                                  }
                              }
                              /* append segment */
                              UINTN sl = StrLen(seg);
                              for (UINTN t = 0; t < sl; t++) Combined[CombinedLen++] = seg[t];
                              Combined[CombinedLen] = L'\0';
                          }
                          CmdLineArg = Combined; /* Use combined list for parsing */
                      }
                  }
                  break;
              }
          }
      }

      /* Try treat the argument as a file path first */
      SHELL_FILE_HANDLE rfile = NULL;
      if (try_open_readonly(CmdLineArg, &rfile)) {
          /* Read file content */
          UINT8 *raw = NULL; UINTN raw_len = 0;
          EFI_STATUS RS = read_all_bytes(rfile, &raw, &raw_len);
          ShellCloseFile(&rfile);
          if (EFI_ERROR(RS)) {
              Print(L"Failed to read rules file %s\n", CmdLineArg);
              if (Combined) gBS->FreePool(Combined);
              return 0;
          }

          CHAR16 *wtext = NULL; UINTN wlen = 0;
          RS = bytes_to_wchars(raw, raw_len, &wtext, &wlen);
          gBS->FreePool(raw);
          if (EFI_ERROR(RS)) {
              Print(L"Failed to decode rules file %s\n", CmdLineArg);
              if (Combined) gBS->FreePool(Combined);
              return 0;
          }

          /* Count tokens and allocate list */
          UINT32 max_tokens = count_rule_tokens_in_text(wtext, wlen);
          if (max_tokens > 0) {
              EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                       max_tokens * sizeof(RULE_ID_e),
                                       (VOID **)&g_rule_list);
              if (EFI_ERROR(EStatus)) {
                  Print(L"Allocate memory for -r failed\n");
                  gBS->FreePool(wtext);
                  if (Combined) gBS->FreePool(Combined);
                  return 0;
              }
          }

          g_rule_count = 0;
          parse_rules_text_into_list(wtext, wlen, g_rule_list, &g_rule_count, max_tokens);
          gBS->FreePool(wtext);
      } else {
          /* Not a file: parse as single command-line CSV */
          UINTN len = StrLen(CmdLineArg);
          UINT32 max_tokens = 1;
          UINTN k;
          for (k = 0; k < len; k++) {
              if (CmdLineArg[k] == L',' ||
                  CmdLineArg[k] == L'\n' ||
                  CmdLineArg[k] == L'\r')
                  max_tokens++;
          }
          if (max_tokens > 0) {
              EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                       max_tokens * sizeof(RULE_ID_e),
                                       (VOID **)&g_rule_list);
              if (EFI_ERROR(EStatus)) {
                  Print(L"Allocate memory for -r failed\n");
                  if (Combined) gBS->FreePool(Combined);
                  return 0;
              }
          }
          UINTN start = 0; g_rule_count = 0;
          while (start < len) {
              UINTN end = start;
              while (end < len &&
                     CmdLineArg[end] != L',' &&
                     CmdLineArg[end] != L'\n' &&
                     CmdLineArg[end] != L'\r')
                end++;
              /* trim spaces/tabs/CR/LF at boundaries */
              while (start < end &&
                     (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                      CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                start++;
              while (end > start &&
                     (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                      CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                end--;
              if (end > start) {
                  CHAR16 wtoken[64];
                  UINTN tlen = end - start;
                  if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                    tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                  for (UINTN ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                  wtoken[tlen] = L'\0';
                  map_and_add_rule_token(wtoken, g_rule_list, &g_rule_count, max_tokens);
              }
              start = (end < len) ? end + 1 : end;
          }
      }

      if (Combined) gBS->FreePool(Combined);
  }

  /* Decide default architecture selection: if neither -r nor -a given, default to -a bsa */
  BOOLEAN DefaultToBsa = FALSE;
  if ((ArchArg == NULL) && (g_rule_count == 0)) {
      DefaultToBsa = TRUE;
      Print(L"No -r or -a specified; defaulting to -a bsa\n");
  }

  /* If -a bsa (or defaulted): merge all BSA rules from lookup table into the rule list */
  if ((ArchArg && w_ascii_streq_caseins(ArchArg, L"bsa")) || DefaultToBsa) {
      /* Count entries in bsa_rule_list until sentinel */
      UINT32 count = 0;
      while (bsa_rule_list[count].rule_id != RULE_ID_SENTINEL) {
          count++;
      }
      if (count == 0) {
          Print(L"No BSA rules in lookup table; skipping -a bsa\n");
      } else {
          /* Merge BSA rules into existing list, deduplicating */
          RULE_ID_e *old_list = g_rule_list;
          UINT32 old_count = g_rule_count;
          EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                   (old_count + count) * sizeof(RULE_ID_e),
                                   (VOID **)&g_rule_list);
          if (EFI_ERROR(EStatus)) {
              Print(L"Allocate memory for -a bsa failed\n");
              g_rule_list = old_list; /* preserve old on failure */
              return 0;
          }
          /* Copy existing */
          for (UINT32 i = 0; i < old_count; i++) g_rule_list[i] = old_list[i];
          UINT32 new_count = old_count;
          /* Append unique from table */
          for (UINT32 i = 0; i < count; i++) {
              RULE_ID_e rid = bsa_rule_list[i].rule_id;
              if (!rule_in_list(rid, g_rule_list, new_count)) g_rule_list[new_count++] = rid;
          }
          if (old_list) gBS->FreePool(old_list);
          g_rule_count = new_count;
      }
  }

  /* If -a sbsa: merge all SBSA rules from lookup table into the rule list */
  if (ArchArg && w_ascii_streq_caseins(ArchArg, L"sbsa")) {
      /* Count entries in sbsa_rule_list until sentinel */
      UINT32 count = 0;
      while (sbsa_rule_list[count].rule_id != RULE_ID_SENTINEL) {
          count++;
      }
      if (count == 0) {
          Print(L"No SBSA rules in lookup table; skipping -a sbsa\n");
      } else {
          /* Merge SBSA rules into existing list, deduplicating */
          RULE_ID_e *old_list = g_rule_list;
          UINT32 old_count = g_rule_count;
          EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                   (old_count + count) * sizeof(RULE_ID_e),
                                   (VOID **)&g_rule_list);
          if (EFI_ERROR(EStatus)) {
              Print(L"Allocate memory for -a sbsa failed\n");
              g_rule_list = old_list; /* preserve old on failure */
              return 0;
          }
          /* Copy existing */
          for (UINT32 i = 0; i < old_count; i++) g_rule_list[i] = old_list[i];
          UINT32 new_count = old_count;
          /* Append unique from table */
          for (UINT32 i = 0; i < count; i++) {
              RULE_ID_e rid = sbsa_rule_list[i].rule_id;
              if (!rule_in_list(rid, g_rule_list, new_count)) g_rule_list[new_count++] = rid;
          }
          if (old_list) gBS->FreePool(old_list);
          g_rule_count = new_count;
      }
  }

  /* If -a pcbsa: merge all PC BSA rules from lookup table into the rule list */
  if (ArchArg && w_ascii_streq_caseins(ArchArg, L"pcbsa")) {
      /* Count entries in pcbsa_rule_list until sentinel */
      UINT32 count = 0;
      while (pcbsa_rule_list[count].rule_id != RULE_ID_SENTINEL) {
          count++;
      }
      if (count == 0) {
          Print(L"No PC BSA rules in lookup table; skipping -a pcbsa\n");
      } else {
          /* Merge PC BSA rules into existing list, deduplicating */
          RULE_ID_e *old_list = g_rule_list;
          UINT32 old_count = g_rule_count;
          EFI_STATUS EStatus = gBS->AllocatePool(EfiBootServicesData,
                                   (old_count + count) * sizeof(RULE_ID_e),
                                   (VOID **)&g_rule_list);
          if (EFI_ERROR(EStatus)) {
              Print(L"Allocate memory for -a pcbsa failed\n");
              g_rule_list = old_list; /* preserve old on failure */
              return 0;
          }
          /* Copy existing */
          for (UINT32 i = 0; i < old_count; i++) g_rule_list[i] = old_list[i];
          UINT32 new_count = old_count;
          /* Append unique from table */
          for (UINT32 i = 0; i < count; i++) {
              RULE_ID_e rid = pcbsa_rule_list[i].rule_id;
              if (!rule_in_list(rid, g_rule_list, new_count)) g_rule_list[new_count++] = rid;
          }
          if (old_list) gBS->FreePool(old_list);
          g_rule_count = new_count;
      }
  }

  // Options with Values
  if (ShellCommandLineGetFlag (ParamPackage, L"-m")) {
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-m");
      if (CmdLineArg == NULL) {
          Print(L"Invalid parameter passed for -m\n", 0);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
      } else {
          /* Count tokens to size the allocation */
          UINTN mlen = StrLen(CmdLineArg);
          UINT32 max_tokens = 1;
          for (UINTN k = 0; k < mlen; k++) {
              if (CmdLineArg[k] == L',' ||
                  CmdLineArg[k] == L'\n' ||
                  CmdLineArg[k] == L'\r')
                  max_tokens++;
          }
          EFI_STATUS est = gBS->AllocatePool(EfiBootServicesData,
                                max_tokens * sizeof(UINT32),
                                (VOID **) &g_execute_modules);
          if (EFI_ERROR(est)) {
              Print(L"Allocate memory for -m failed\n", 0);
              return 0;
          }

          /* Parse CSV of module names */
          g_num_modules = 0;
          UINTN start = 0;
          while (start < mlen) {
              UINTN end = start;
              while (end < mlen &&
                     CmdLineArg[end] != L',' &&
                     CmdLineArg[end] != L'\n' &&
                     CmdLineArg[end] != L'\r')
                end++;
              /* Trim boundaries */
              while (start < end &&
                     (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                      CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                start++;
              while (end > start &&
                     (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                      CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                end--;
              if (end > start) {
                  CHAR16 wtoken[32];
                  UINTN tlen = end - start;
                  if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                    tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                  for (UINTN ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                  wtoken[tlen] = L'\0';
                  map_and_add_module_token(wtoken, g_execute_modules, &g_num_modules, max_tokens);
              }
              start = (end < mlen) ? end + 1 : end;
          }

          if (g_num_modules == 0) {
              Print(L"No valid modules parsed from -m\n", 0);
          }
      }
  }

  /* Options with Values: -skipmodule <comma-separated module names to skip> */
  if (ShellCommandLineGetFlag (ParamPackage, L"-skipmodule")) {
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skipmodule");
      if (CmdLineArg == NULL) {
          Print(L"Invalid parameter passed for -skipmodule\n", 0);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
      } else {
          UINTN mlen = StrLen(CmdLineArg);
          UINT32 max_tokens = 1;
          for (UINTN k = 0; k < mlen; k++) {
              if (CmdLineArg[k] == L',' ||
                  CmdLineArg[k] == L'\n' ||
                  CmdLineArg[k] == L'\r')
                  max_tokens++;
          }
          EFI_STATUS est = gBS->AllocatePool(EfiBootServicesData,
                                max_tokens * sizeof(UINT32),
                                (VOID **) &g_skip_modules);
          if (EFI_ERROR(est)) {
              Print(L"Allocate memory for -skipmodule failed\n", 0);
              return 0;
          }

          g_num_skip_modules = 0;
          UINTN start = 0;
          while (start < mlen) {
              UINTN end = start;
              while (end < mlen &&
                     CmdLineArg[end] != L',' &&
                     CmdLineArg[end] != L'\n' &&
                     CmdLineArg[end] != L'\r')
                end++;
              /* Trim boundaries */
              while (start < end &&
                     (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                      CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                start++;
              while (end > start &&
                     (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                      CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                end--;
              if (end > start) {
                  CHAR16 wtoken[32];
                  UINTN tlen = end - start;
                  if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                    tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                  for (UINTN ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                  wtoken[tlen] = L'\0';
                  map_and_add_module_token(wtoken, g_skip_modules, &g_num_skip_modules, max_tokens);
              }
              start = (end < mlen) ? end + 1 : end;
          }

          if (g_num_skip_modules == 0) {
              Print(L"No valid modules parsed from -skipmodule\n", 0);
          }
      }
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-p2p")) {
    ;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-skip-dp-nic-ms")) {
    g_pcie_skip_dp_nic_ms = TRUE;
  } else {
    g_pcie_skip_dp_nic_ms = FALSE;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-p2p")) {
    g_pcie_p2p = TRUE;
  } else {
    g_pcie_p2p = FALSE;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-cache")) {
    g_pcie_cache_present = TRUE;
  } else {
    g_pcie_cache_present = FALSE;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-el1physkip")) {
    g_el1physkip = TRUE;
  }
  //
  // Initialize global counters
  //
  g_acs_tests_total = 0;
  g_acs_tests_pass  = 0;
  g_acs_tests_fail  = 0;

  return(0);
}

#define BSA_LEVEL_PRINT_FORMAT(level, only) ((level > BSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

#define SBSA_LEVEL_PRINT_FORMAT(level, only) ((level > SBSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

#define PC_BSA_LEVEL_PRINT_FORMAT(level, only) ((level > PCBSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

STATIC VOID FlushImage (VOID)
{
  EFI_LOADED_IMAGE_PROTOCOL   *ImageInfo;
  EFI_STATUS Status;
  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
  if(EFI_ERROR (Status))
  {
    return;
  }

  val_pe_cache_clean_range((UINT64)ImageInfo->ImageBase, (UINT64)ImageInfo->ImageSize);

}

UINT32
createPeInfoTable (
)
{
  UINT32 Status;
  UINT64 *PeInfoTable;

  PeInfoTable = val_aligned_alloc(SIZE_4K, PE_INFO_TBL_SZ);

  Status = val_pe_create_info_table(PeInfoTable);

  return Status;
}

UINT32
createGicInfoTable (
)
{
  UINT32 Status;
  UINT64 *GicInfoTable;

  GicInfoTable = val_aligned_alloc(SIZE_4K, GIC_INFO_TBL_SZ);

  Status = val_gic_create_info_table(GicInfoTable);

  return Status;
}

VOID
createTimerInfoTable(
)
{
  UINT64 *TimerInfoTable;

  TimerInfoTable = val_aligned_alloc(SIZE_4K, TIMER_INFO_TBL_SZ);

  val_timer_create_info_table(TimerInfoTable);
}

VOID
createWatchdogInfoTable(
)
{
  UINT64 *WdInfoTable;

  WdInfoTable = val_aligned_alloc(SIZE_4K, WD_INFO_TBL_SZ);

  val_wd_create_info_table(WdInfoTable);
}


VOID
createPcieVirtInfoTable(
)
{
  UINT64 *PcieInfoTable;
  UINT64 *IoVirtInfoTable;

  PcieInfoTable   = val_aligned_alloc(SIZE_4K, PCIE_INFO_TBL_SZ);

  val_pcie_create_info_table(PcieInfoTable);

  IoVirtInfoTable = val_aligned_alloc(SIZE_4K, IOVIRT_INFO_TBL_SZ);

  val_iovirt_create_info_table(IoVirtInfoTable);
}

VOID
createPeripheralInfoTable(
)
{
  UINT64 *PeripheralInfoTable;
  UINT64 *MemoryInfoTable;

  PeripheralInfoTable = val_aligned_alloc(SIZE_4K, PERIPHERAL_INFO_TBL_SZ);

  val_peripheral_create_info_table(PeripheralInfoTable);

  MemoryInfoTable = val_aligned_alloc(SIZE_4K, MEM_INFO_TBL_SZ);

  val_memory_create_info_table(MemoryInfoTable);
}

VOID
createSmbiosInfoTable(
)
{
  UINT64 *SmbiosInfoTable;

  SmbiosInfoTable = val_aligned_alloc(SIZE_4K, SMBIOS_INFO_TBL_SZ);

  val_smbios_create_info_table(SmbiosInfoTable);
}

VOID
createPmuInfoTable(
)
{
  UINT64 *PmuInfoTable;

  PmuInfoTable = val_aligned_alloc(SIZE_4K, PMU_INFO_TBL_SZ);

  val_pmu_create_info_table(PmuInfoTable);
}

UINT32
createRasInfoTable(
)
{
  UINT32 status;
  UINT64 *RasInfoTable;

  RasInfoTable = val_aligned_alloc(SIZE_4K, RAS_INFO_TBL_SZ);

  status = val_ras_create_info_table(RasInfoTable);

  return status;
}

VOID
createCacheInfoTable(
)
{
  UINT64 *CacheInfoTable;

  CacheInfoTable = val_aligned_alloc(SIZE_4K, CACHE_INFO_TBL_SZ);

  val_cache_create_info_table(CacheInfoTable);
}

VOID
createMpamInfoTable(
)
{
  UINT64 *MpamInfoTable;

  MpamInfoTable = val_aligned_alloc(SIZE_4K, MPAM_INFO_TBL_SZ);

  val_mpam_create_info_table(MpamInfoTable);
}

VOID
createHmatInfoTable(
)
{
  UINT64 *HmatInfoTable;

  HmatInfoTable = val_aligned_alloc(SIZE_4K, HMAT_INFO_TBL_SZ);

  val_hmat_create_info_table(HmatInfoTable);
}

VOID
createSratInfoTable(
)
{
  UINT64 *SratInfoTable;

  SratInfoTable = val_aligned_alloc(SIZE_4K, SRAT_INFO_TBL_SZ);

  val_srat_create_info_table(SratInfoTable);
}

VOID
createPccInfoTable(
)
{
  UINT64 *PccInfoTable;

  PccInfoTable = val_aligned_alloc(SIZE_4K, PCC_INFO_TBL_SZ);

  val_pcc_create_info_table(PccInfoTable);
}

VOID
createRas2InfoTable(
)
{
  UINT64 *Ras2InfoTable;

  Ras2InfoTable = val_aligned_alloc(SIZE_4K, RAS2_FEAT_INFO_TBL_SZ);

  val_ras2_create_info_table(Ras2InfoTable);
}

VOID
createTpm2InfoTable(
)
{
  UINT64 *Tpm2InfoTable;

  Tpm2InfoTable = val_aligned_alloc(SIZE_4K, TPM2_INFO_TBL_SZ);

  val_tpm2_create_info_table(Tpm2InfoTable);
}



VOID
freeBsaAcsMem()
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

  val_print(ACS_PRINT_TEST, "\n\n BSA Architecture Compliance Suite", 0);
  val_print(ACS_PRINT_TEST, "\n          Version %d.", BSA_ACS_MAJOR_VER);
  val_print(ACS_PRINT_TEST, "%d.", BSA_ACS_MINOR_VER);
  val_print(ACS_PRINT_TEST, "%d\n", BSA_ACS_SUBMINOR_VER);


  val_print(ACS_PRINT_TEST, BSA_LEVEL_PRINT_FORMAT(g_bsa_level, g_bsa_only_level),
                                   (g_bsa_level > BSA_MAX_LEVEL_SUPPORTED) ? 0 : g_bsa_level);

  if (g_bsa_only_level)
    g_bsa_level = 0;

  val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);
  val_print(ACS_PRINT_TEST, "\n       Creating Platform Information Tables\n", 0);


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

  /* If user provided -r rules, run rule-based executor and return */
  if (g_rule_count > 0 && g_rule_list != NULL) {
    /* Apply CLI filters (-skip, -m, -skipmodule) to the parsed list */
    g_rule_count = filter_rule_list_by_cli(g_rule_list, g_rule_count);

    /* Print all rule IDs to be executed after filtering for verification */
    val_print(ACS_PRINT_TEST, "\nSelected rules (-r): ", 0);
    for (UINT32 i = 0; i < g_rule_count; i++) {
      RULE_ID_e rid = g_rule_list[i];
      if (rid < RULE_ID_SENTINEL && rule_id_string[rid] != NULL) {
        val_print(ACS_PRINT_TEST, (char8_t *)rule_id_string[rid], 0);
      } else {
        val_print(ACS_PRINT_TEST, "<INVALID>", 0);
      }
      if (i + 1 < g_rule_count)
        val_print(ACS_PRINT_TEST, ",", 0);
    }
    val_print(ACS_PRINT_TEST, "\n", 0);

    /* Also print skipped rules (-skip) if provided */
    if (g_skip_rule_count > 0 && g_skip_rule_list != NULL) {
      val_print(ACS_PRINT_TEST, "Skipped rules (-skip): ", 0);
      for (UINT32 i = 0; i < g_skip_rule_count; i++) {
        RULE_ID_e rid = g_skip_rule_list[i];
        if (rid < RULE_ID_SENTINEL && rule_id_string[rid] != NULL) {
          val_print(ACS_PRINT_TEST, (char8_t *)rule_id_string[rid], 0);
        } else {
          val_print(ACS_PRINT_TEST, "<INVALID>", 0);
        }
        if (i + 1 < g_skip_rule_count)
          val_print(ACS_PRINT_TEST, ",", 0);
      }
      val_print(ACS_PRINT_TEST, "\n", 0);
    }

    /* Also print selected modules (-m) if provided */
    if (g_num_modules > 0 && g_execute_modules != NULL) {
      val_print(ACS_PRINT_TEST, "Selected modules (-m): ", 0);
      for (UINT32 i = 0; i < g_num_modules; i++) {
        UINT32 mid = g_execute_modules[i];
        if (mid < MODULE_ID_SENTINEL && module_name_string[mid] != NULL) {
          val_print(ACS_PRINT_TEST, (char8_t *)module_name_string[mid], 0);
        } else {
          val_print(ACS_PRINT_TEST, "<INVALID>", 0);
        }
        if (i + 1 < g_num_modules)
          val_print(ACS_PRINT_TEST, ",", 0);
      }
      val_print(ACS_PRINT_TEST, "\n", 0);
    }

    /* And print skipped modules (-skipmodule) if provided */
    if (g_num_skip_modules > 0 && g_skip_modules != NULL) {
      val_print(ACS_PRINT_TEST, "Skipped modules (-skipmodule): ", 0);
      for (UINT32 i = 0; i < g_num_skip_modules; i++) {
        UINT32 mid = g_skip_modules[i];
        if (mid < MODULE_ID_SENTINEL && module_name_string[mid] != NULL) {
          val_print(ACS_PRINT_TEST, (char8_t *)module_name_string[mid], 0);
        } else {
          val_print(ACS_PRINT_TEST, "<INVALID>", 0);
        }
        if (i + 1 < g_num_skip_modules)
          val_print(ACS_PRINT_TEST, ",", 0);
      }
      val_print(ACS_PRINT_TEST, "\n", 0);
    }

    run_tests(g_rule_list, g_rule_count);
    return 0;
  }

  val_bsa_execute_tests(g_sw_view);

  if (g_build_sbsa) {

      val_print(ACS_PRINT_ERR, "\n      *** BSA tests complete. Running SBSA Tests. ***\n\n\n", 0);
      val_print(ACS_PRINT_ERR, "\n\n SBSA Architecture Compliance Suite\n", 0);
      val_print(ACS_PRINT_ERR, "    Version %d.", SBSA_ACS_MAJOR_VER);
      val_print(ACS_PRINT_ERR, "%d.", SBSA_ACS_MINOR_VER);
      val_print(ACS_PRINT_ERR, "%d\n", SBSA_ACS_SUBMINOR_VER);

      val_print(ACS_PRINT_TEST, SBSA_LEVEL_PRINT_FORMAT(g_sbsa_level, g_sbsa_only_level),
                                   (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) ? 0 : g_sbsa_level);

      val_sbsa_execute_tests(g_sbsa_level);
  }

  if (g_build_pcbsa) {

      val_print(ACS_PRINT_ERR, "\n      *** BSA tests complete. Running PC BSA Tests. ***\n\n", 0);
      val_print(ACS_PRINT_ERR, "\n\n PC BSA Architecture Compliance Suite\n", 0);
      val_print(ACS_PRINT_ERR, "    Version %d.", PC_BSA_ACS_MAJOR_VER);
      val_print(ACS_PRINT_ERR, "%d.", PC_BSA_ACS_MINOR_VER);
      val_print(ACS_PRINT_ERR, "%d\n", PC_BSA_ACS_SUBMINOR_VER);

      val_print(ACS_PRINT_TEST, PC_BSA_LEVEL_PRINT_FORMAT(g_pcbsa_level, g_pcbsa_only_level),
                                   (g_pcbsa_level > PCBSA_MAX_LEVEL_SUPPORTED) ? 0 : g_pcbsa_level);

      if (g_pcbsa_only_level)
          g_pcbsa_level = 0;

      val_pcbsa_execute_tests(g_pcbsa_level);
  }

  /* Initialise exception vector, so any unexpected exception gets handled by default
     BSA exception handler */
  branch_label = &&print_test_status;
  val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);

print_test_status:
  val_print(ACS_PRINT_ERR, "\n     -------------------------------------------------------\n", 0);
  val_print(ACS_PRINT_ERR, "     Total Tests run  = %4d", g_acs_tests_total);
  val_print(ACS_PRINT_ERR, "  Tests Passed  = %4d", g_acs_tests_pass);
  val_print(ACS_PRINT_ERR, "  Tests Failed = %4d\n", g_acs_tests_fail);
  val_print(ACS_PRINT_ERR, "     -------------------------------------------------------\n", 0);

  freeBsaAcsMem();

  if (g_dtb_log_file_handle) {
    ShellCloseFile(&g_dtb_log_file_handle);
  }

  if (g_build_sbsa)
      val_print(ACS_PRINT_ERR, "\n      *** SBSA tests complete. Reset the system. ***\n\n", 0);
  else if (g_build_pcbsa)
      val_print(ACS_PRINT_ERR, "\n      *** PC BSA tests complete. Reset the system. ***\n\n", 0);
  else
      val_print(ACS_PRINT_ERR, "\n      *** BSA tests complete. Reset the system. ***\n\n", 0);

  if (g_acs_log_file_handle) {
    ShellCloseFile(&g_acs_log_file_handle);
  }

  val_pe_context_restore(AA64WriteSp(g_stack_pointer));
  return 0;
}
