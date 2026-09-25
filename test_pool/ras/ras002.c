/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#include "acs_val.h"
#include "val_interface.h"
#include "acs_memory.h"
#include "acs_pe.h"
#include "acs_ras.h"
#include "acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 2)
#define TEST_RULE  "RAS_02"
#define TEST_DESC  "Check CFI, DUI, UI Controls           "

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0;
  uint32_t warn_cnt = 0;
  uint32_t checked_rec_cnt = 0;
  uint32_t applicable_node_cnt = 0;
  uint64_t node_type;
  uint64_t num_node;
  uint64_t value;
  uint32_t node_index;
  uint32_t err_rec_idx;
  uint64_t num_err_recs;
  uint64_t cfi;
  uint64_t dui;
  uint64_t ui;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(DEBUG, "\n       No RAS Nodes found in AEST table.");
    val_print(DEBUG, "\n       Consider test fail if system support RAS nodes");
    val_set_status(index, RESULT_WARNING(01));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get Current Node Type */
    status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &value);
    if (status) {
      val_print(DEBUG, "\n       Node Type not found index %d", node_index);
      fail_cnt++;
      break;
    }

    node_type = value;

    /* Check if Node is Memory Controller/PE (Cache Resource) */
    if (!((node_type == NODE_TYPE_MC) || (node_type == NODE_TYPE_PE)))
        continue;

    /* Check for Cache Resource Type in case of Processor Node */
    if (node_type == NODE_TYPE_PE) {
      status = val_ras_get_info(RAS_INFO_PE_RES_TYPE, node_index, &value);
      if (status) {
        val_print(DEBUG, "\n       PE Resource type not found index %d", node_index);
        fail_cnt++;
        break;
      }

      if (value != 0)
        continue;
    }

    applicable_node_cnt++;

    /* Get Error Record number for this Node */
    status = val_ras_get_info(RAS_INFO_NUM_ERR_REC, node_index, &num_err_recs);
    if (status) {
        val_print(ERROR, "\n       Unable to get error record count for RAS node %d",
                  node_index);
        fail_cnt++;
        continue;
    }

    if (num_err_recs == 0) {
        val_print(DEBUG, "\n       RAS node %d has no error records", node_index);
        continue;
    }

    /* Enumerate all implemented Error Records */
    for (err_rec_idx = 0; err_rec_idx < num_err_recs; err_rec_idx++) {
      /* Read FR register of the current error record */
      value = val_ras_reg_read(node_index, RAS_ERR_FR, err_rec_idx);
      val_print(DEBUG, "\n       ERR<%d>FR = 0x%llx", err_rec_idx, value);
      if (value == INVALID_RAS_REG_VAL) {
          val_print(ERROR, "\n       Couldn't read ERR<%d>FR register", err_rec_idx);
          val_print(ERROR, "\n       RAS node index: %d", node_index);
          fail_cnt++;
          continue;
      }

      checked_rec_cnt++;

      if (!(value & ERR_FR_FRX_MASK)) {
          val_print(WARN, "\n       ERR<%d>FR.FRX is 0 for RAS node %d", err_rec_idx, node_index);
          val_print(WARN, "\n       Unable to determine recorded error states");
          warn_cnt++;
          continue;
      }

      /*
       * If the node records Deferred errors, DUI must be
       * either 0b10 or 0b11.
       */
      if (value & ERR_FR_DE_MASK) {
        dui = (value & ERR_FR_DUI_MASK) >> 16;
        if ((dui != 0x2) && (dui != 0x3)) {
          if (node_type == NODE_TYPE_MC) {
              val_print(ERROR,
                        "\n       DUI must be 0b10 or 0b11 for node_index %d",
                        node_index);
              fail_cnt++;
          } else {
              val_print(WARN,
                        "\n       DUI must be 0b10 or 0b11 for cache node_index %d",
                        node_index);
              warn_cnt++;
          }
        }
      }

      /*
       * If the node records corrected errors, CFI must be
       * either 0b10 or 0b11.
       */
      if (value & ERR_FR_CE_MASK) {
        cfi = (value & ERR_FR_CFI_MASK) >> 10;
        if ((cfi != 0x2) && (cfi != 0x3)) {
          if (node_type == NODE_TYPE_MC) {
              val_print(ERROR,
                        "\n       CFI must be 0b10 or 0b11 for node_index %d",
                        node_index);
              fail_cnt++;
          } else {
              val_print(WARN,
                        "\n       CFI must be 0b10 or 0b11 for cache node_index %d",
                        node_index);
              warn_cnt++;
          }
        }
      }

      /*
      * UC, UEU, UER and UEO are the sub-types of an
      * Uncorrected component error state. If the node records
      * any Uncorrected error type, UI must implement the
      * required control (0b10 or 0b11).
      */
      if (value & (ERR_FR_UEO_MASK | ERR_FR_UER_MASK | ERR_FR_UEU_MASK | ERR_FR_UC_MASK)) {
        ui = (value & ERR_FR_UI_MASK) >> 4;
        if ((ui != 0x2) && (ui != 0x3)) {
          if (node_type == NODE_TYPE_MC) {
              val_print(ERROR,
                        "\n       UI must be 0b10 or 0b11 for node_index %d",
                        node_index);
              fail_cnt++;
          } else {
              val_print(WARN,
                        "\n       UI must be 0b10 or 0b11 for cache node_index %d",
                        node_index);
              warn_cnt++;
          }
        }
      }
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(01));
    return;
  }

  if (warn_cnt) {
    val_set_status(index, RESULT_WARNING(02));
    return;
  }

  if (applicable_node_cnt == 0) {
      val_print(DEBUG, "\n       No applicable Memory Controller or Cache RAS nodes found");
      val_set_status(index, RESULT_SKIP(03));
      return;
  }

  if (checked_rec_cnt == 0) {
      val_print(WARN, "\n       No applicable RAS error records available for validation");
      val_set_status(index, RESULT_WARNING(03));
      return;
  }

  val_set_status(index, RESULT_PASS);
}

uint32_t
ras002_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
