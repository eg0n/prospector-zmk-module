/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Record a peripheral's link state, then raise split_central_status_changed.
 * Widgets read the recorded state, as events close together can coalesce. */
void prospector_peripheral_status_set(uint8_t slot, bool connected);

bool prospector_peripheral_connected(uint8_t slot);
