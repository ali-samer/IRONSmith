#include "aieplugin/design/DesignModel.hpp"

namespace Aie::Internal {

bool DesignModel::hasDesignState() const
{
    return !design.isEmpty();
}

} // namespace Aie::Internal
