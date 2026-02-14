#pragma once

#include "aieplugin/design/DesignModel.hpp"
#include "aieplugin/NpuProfile.hpp"

#include <utils/Result.hpp>

#include <QtCore/QStringView>

namespace Aie::Internal {

class DesignBundleLoader final
{
public:
    explicit DesignBundleLoader(const Aie::NpuProfileCatalog* catalog);

    Utils::Result load(const QString& bundlePath, DesignModel& outModel) const;

private:
    Utils::Result resolveArchForDeviceFamily(const QString& deviceFamily, QString& outArch) const;
    Utils::Result resolveProfileForArch(const QString& arch, const Aie::NpuProfile*& outProfile) const;
    static bool archMatches(QStringView lhs, QStringView rhs);

    const Aie::NpuProfileCatalog* m_catalog = nullptr;
};

} // namespace Aie::Internal
