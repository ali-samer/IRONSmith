// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/StrongId.hpp"

namespace Utils {

struct TreeNodeIdTag final {};
using TreeNodeId = StrongId<TreeNodeIdTag>;

} // namespace Utils
