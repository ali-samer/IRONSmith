#pragma once

#include "utils/StrongId.hpp"

namespace Utils {

struct TreeNodeIdTag final {};
using TreeNodeId = StrongId<TreeNodeIdTag>;

} // namespace Utils
