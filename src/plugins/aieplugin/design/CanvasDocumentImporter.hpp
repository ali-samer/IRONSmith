#pragma once

#include "aieplugin/design/DesignModel.hpp"

#include <utils/Result.hpp>

namespace Aie::Internal {

class AieService;

class CanvasDocumentImporter final
{
public:
    explicit CanvasDocumentImporter(AieService* service);

    Utils::Result importDesign(const DesignModel& model) const;

private:
    Utils::Result applyDesignState(const DesignModel& model) const;

    AieService* m_service = nullptr;
};

} // namespace Aie::Internal
