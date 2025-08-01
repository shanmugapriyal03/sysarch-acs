/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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

#ifndef __ACS_TPM2_H
#define __ACS_TPM2_H

/* -------- TPM FIFO (TIS) Interface -------- */
#define TPM_FIFO_BASE_ADDRESS             0xFED40000  /* Default FIFO base address (TIS) */
#define TPM_FIFO_INTERFACE_ID_OFFSET      0x0030      /* Offset to TPM_INTERFACE_ID_0 register */


/* -------- TPM CRB Interface -------- */
#define TPM_CRB_INTERFACE_ID_OFFSET       0x0030      /* Offset to TPM_CRB_INTF_ID_0 register */
#define TPM_CRB_CONTROL_AREA_OFFSET       0x0040      /* Offset to CRB Control Area Base */


/* TPM Start Method */

#define TPM_IF_START_METHOD_ACPI                          2
#define TPM_IF_START_METHOD_TIS                           6
#define TPM_IF_START_METHOD_CRB                           7
#define TPM_IF_START_METHOD_CRB_ACPI                      8
#define TPM_IF_START_METHOD_CRB_SMC                       11
#define TPM_IF_START_METHOD_CRB_FFA                       15

uint32_t tpm001_entry(uint32_t num_pe);
uint32_t tpm002_entry(uint32_t num_pe);

#endif // __ACS_TPM2_H
