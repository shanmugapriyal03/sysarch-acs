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

#include "include/rule_based_execution.h"

extern rule_test_map_t rule_test_map[RULE_ID_SENTINEL];
extern char8_t *rule_id_string[RULE_ID_SENTINEL];
extern uint32_t alias_rule_map_count;
extern alias_rule_map_t alias_rule_map[];

/**
 * @brief Check if a rule ID exists in a list.
 *
 * Linear scan of the first `count` entries of `list` to see if any equals
 * `rid`.
 *
 * @param rid   Rule ID to find.
 * @param list  Rule ID array to scan.
 * @param count Number of valid entries in `list`.
 * @return 1 if found; 0 otherwise.
 */
bool rule_in_list(RULE_ID_e rid, const RULE_ID_e *list, uint32_t count)
{
    if (!list || count == 0)
        return 0;
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] == rid)
            return 1;
    }
    return 0;
}

/**
 * @brief Bounded ASCII string length.
 *
 * Counts characters in a NUL-terminated ASCII string up to a maximum bound
 * without relying on libc. If `s` is NULL, returns 0.
 *
 * @param s   Pointer to input string (may be NULL).
 * @param max Maximum number of characters to scan.
 * @return uint32_t Number of characters found before NUL or `max`,
 *                  whichever occurs first.
 */
static uint32_t ascii_strnlen(const char8_t *s, uint32_t max)
{
    if (!s) return 0;
    uint32_t n = 0;
    while (n < max && s[n] != '\0') n++;
    return n;
}

/**
 * @brief In-place quicksort for rule ID arrays.
 *
 * Sorts an array of `RULE_ID_e` values in ascending order using an
 * iterative quicksort (no recursion). The implementation processes the
 * smaller partition first to keep the auxiliary stack depth bounded.
 *
 * @param rule_list Pointer to the array of `RULE_ID_e` to sort.
 * @param list_size Number of elements in `rule_list`.
 */
void quick_sort_rule_list(RULE_ID_e *rule_list, uint32_t list_size)
{
    if (!rule_list || list_size < 2) return;          /* quick exit */

    /* Small fixed size stack (32 pairs handle any 32bit length) */
    int lo_stack[32];
    int hi_stack[32];
    int top = 0;

    lo_stack[0] = 0;
    hi_stack[0] = (int)list_size - 1;

    while (top >= 0)
    {
        int lo = lo_stack[top];
        int hi = hi_stack[top];
        --top;

        while (lo < hi)
        {
            /* Partition step */
            RULE_ID_e pivot = rule_list[hi];
            int i = lo - 1;

            for (int j = lo; j < hi; ++j)
            {
                if (rule_list[j] <= pivot)
                {
                    ++i;
                    RULE_ID_e tmp = rule_list[i];
                    rule_list[i] = rule_list[j];
                    rule_list[j] = tmp;
                }
            }

            ++i;                                    /* pivot's final spot */
            RULE_ID_e tmp = rule_list[i];
            rule_list[i] = rule_list[hi];
            rule_list[hi] = tmp;

            /* Handle smaller side first to control stack depth */
            if (i - lo < hi - i)
            {
                /* Push right half */
                if (i + 1 < hi) {
                    ++top;
                    lo_stack[top] = i + 1;
                    hi_stack[top] = hi;
                }
                hi = i - 1;                          /* loop on left half */
            }
            else
            {
                /* Push left half */
                if (lo < i - 1) {
                    ++top;
                    lo_stack[top] = lo;
                    hi_stack[top] = i - 1;
                }
                lo = i + 1;                          /* loop on right half */
            }
        }
    }
}


/**
 * @brief Print an aligned rule header line before running a test.
 *
 * Formats and emits a single line containing the rule ID and description
 * in fixed-width columns without using printf-style width specifiers.
 * The rule ID is padded/truncated to `RULE_STRING_SIZE` and the
 * description to `RULE_DESC_SIZE`. Output is assembled into one buffer
 * and printed with a single `val_print` call to preserve alignment.
 *
 * Example output (widths exaggerated for illustration):
 * "\nB_PE_01    : Check Arch symmetry across PE           "
 *
 * @param rule_enum Rule identifier (enum) whose ID string and description
 *                  will be printed.
 * @param indent    Indentation level; output is prefixed by (indent * 4)
 *                  spaces, capped internally to a safe maximum.
 */
void
print_rule_test_start(uint32_t rule_enum, uint32_t indent)
{
    /* Fixed-width, space-padded buffers (+1 for terminator) */
    char8_t id_pad[RULE_STRING_SIZE + 1];
    char8_t desc_pad[RULE_DESC_SIZE + 1];

    /* Build a single line: "\n<indent> <ID_PAD> : <DESC_PAD>\0"
     * Allow up to 16 indentation levels (16*4 = 64 spaces) for safety. */
    #define MAX_INDENT_SPACES 64
    char8_t line[1 /*\n*/ + MAX_INDENT_SPACES + RULE_STRING_SIZE +
                 3 /*" : "*/ + RULE_DESC_SIZE + 1 /*\0*/];

    uint32_t i;

    /* 1) Copy or space-pad rule ID to RULE_STRING_SIZE */
    uint32_t id_len = ascii_strnlen(rule_id_string[rule_enum], RULE_STRING_SIZE);
    for (i = 0; i < id_len; i++) id_pad[i] = rule_id_string[rule_enum][i];
    for (; i < RULE_STRING_SIZE; i++) id_pad[i] = ' ';
    id_pad[RULE_STRING_SIZE] = '\0';

    /* 2) Copy or space-pad description to RULE_DESC_SIZE */
    uint32_t desc_len = ascii_strnlen(rule_test_map[rule_enum].rule_desc, RULE_DESC_SIZE);
    for (i = 0; i < desc_len; i++) desc_pad[i] = rule_test_map[rule_enum].rule_desc[i];
    for (; i < RULE_DESC_SIZE; i++) desc_pad[i] = ' ';
    desc_pad[RULE_DESC_SIZE] = '\0';

    /* 3) Assemble the final output line */
    {
        uint32_t p = 0;
        uint32_t j;

        line[p++] = '\n';
        /* Apply indentation: 4 spaces per indent level, capped */
        uint32_t indent_spaces = indent * 4;
        if (indent_spaces > MAX_INDENT_SPACES) indent_spaces = MAX_INDENT_SPACES;
        for (j = 0; j < indent_spaces && p < sizeof(line)-1; j++) line[p++] = ' ';
        for (j = 0; j < RULE_STRING_SIZE; j++) line[p++] = id_pad[j];
        line[p++] = ' ';
        line[p++] = ':';
        line[p++] = ' ';
        for (j = 0; j < RULE_DESC_SIZE; j++) line[p++] = desc_pad[j];
        line[p] = '\0';
    }

    /* 4) Print once to avoid interleaving and preserve alignment */
    val_print(ACS_PRINT_TEST, line, 0);
}

/**
 * @brief Print the status for a rule test result.
 *
 * Emits a standardized, aligned status suffix (PASS/FAIL/SKIP/WARN/NO SUPPORT)
 * or a hexadecimal code for unknown statuses. Intended to be printed after
 * the rule header line.
 *
 * @param rule_enum Rule identifier (unused here; kept for symmetry/signature).
 * @param status    Result code (e.g. `TEST_PASS`, `TEST_FAIL`).
 */
void
print_rule_test_status(uint32_t rule_enum, uint32_t status)
{
    (void) rule_enum;
    switch (status) {
    case TEST_PASS:
        val_print(ACS_PRINT_TEST, " : Result:  PASS", 0);
        break;
    case TEST_FAIL:
        val_print(ACS_PRINT_TEST, " : Result:  FAIL", 0);
        break;
    case TEST_SKIP:
        val_print(ACS_PRINT_TEST, " : Result:  SKIP", 0);
        break;
    case TEST_WARN:
        val_print(ACS_PRINT_TEST, " : Result:  WARN", 0);
        break;
    case TEST_NS:
        val_print(ACS_PRINT_TEST, " : Result:  NO SUPPORT", 0);
        break;
    default:
        val_print(ACS_PRINT_TEST, " : Result:  0x%x", status);
    }
    return;
}

/**
 * @brief Reset all rule execution statuses to unknown.
 *
 * Initializes the global `rule_status_map` so that each entry corresponding
 * to a valid `RULE_ID_e` is set to `TEST_STATUS_UNKNOWN`. Call this before
 * starting a run to ensure clean status reporting.
 */
void rule_status_map_reset(void)
{
    uint32_t i;
    for (i = 0; i < RULE_ID_SENTINEL; i++) {
        rule_status_map[i] = TEST_STATUS_UNKNOWN;
    }
}

/**
 * @brief Get the index of an alias rule in alias_rule_map.
 *
 * Performs a linear scan over the compile-time `alias_rule_map` and returns
 * the zero-based index of the entry whose `alias_rule_id` matches the input.
 * If no entry matches, returns `INVALID_IDX`.
 *
 * @param alias_rule_id Alias rule identifier to look up.
 * @return uint32_t Zero-based index in `alias_rule_map`, or `INVALID_IDX` if not found.
 */
uint32_t
alias_rule_map_get_index(RULE_ID_e alias_rule_id)
{

    uint32_t i;

    /* Iterate over all entries in the alias map */
    for (i = 0; i <  alias_rule_map_count; i++) {
        /* Check if the current entry's alias matches the requested ID */
        if (alias_rule_map[i].alias_rule_id == alias_rule_id) {
            return i;  /* found: return its index */
        }
    }

    /* Not found: return sentinel indicating absence */
    return INVALID_IDX;
}
