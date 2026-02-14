#include "aieplugin/design/DesignBundleLoader.hpp"

#include "aieplugin/AieConstants.hpp"
#include "aieplugin/NpuProfileLoader.hpp"

#include <utils/DocumentBundle.hpp>
#include <QtCore/QFileInfo>

namespace Aie::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kArchAieMl = u"AIE-ML"_s;
const QString kFamilyAieMl = u"aie-ml"_s;
const QString kFamilyAieMlV2 = u"aie-ml-v2"_s;

QString normalizeToken(const QString& input)
{
    QString out = input.trimmed().toLower();
    out.replace(u'_', u'-');
    return out;
}

} // namespace

DesignBundleLoader::DesignBundleLoader(const Aie::NpuProfileCatalog* catalog)
    : m_catalog(catalog)
{
}

Utils::Result DesignBundleLoader::load(const QString& bundlePath, DesignModel& outModel) const
{
    outModel = DesignModel{};
    if (!m_catalog)
        return Utils::Result::failure(QStringLiteral("NPU profile catalog is not available."));

    QString validateError;
    if (!Utils::DocumentBundle::validate(bundlePath, &validateError))
        return Utils::Result::failure(validateError.isEmpty() ? QStringLiteral("Invalid design bundle.") : validateError);

    const QString normalizedPath = Utils::DocumentBundle::normalizeBundlePath(bundlePath);

    QString error;
    const QJsonObject program = Utils::DocumentBundle::readProgram(normalizedPath, &error);
    if (!error.isEmpty())
        return Utils::Result::failure(error);
    if (program.isEmpty())
        return Utils::Result::failure(QStringLiteral("Program configuration is empty."));

    const QJsonObject design = Utils::DocumentBundle::readDesign(normalizedPath, &error);
    if (!error.isEmpty())
        return Utils::Result::failure(error);

    const QJsonObject manifest = Utils::DocumentBundle::readManifest(normalizedPath, &error);
    if (!error.isEmpty())
        return Utils::Result::failure(error);

    const QString deviceFamily = program.value(QStringLiteral("deviceFamily")).toString();
    if (deviceFamily.trimmed().isEmpty())
        return Utils::Result::failure(QStringLiteral("Program config missing deviceFamily."));

    QString arch;
    Utils::Result archResult = resolveArchForDeviceFamily(deviceFamily, arch);
    if (!archResult)
        return archResult;

    const Aie::NpuProfile* profile = nullptr;
    Utils::Result profileResult = resolveProfileForArch(arch, profile);
    if (!profileResult)
        return profileResult;

    TileCounts counts;
    counts.columns = profile->grid.columns;
    counts.shimRows = profile->grid.rows.shim;
    counts.memRows = profile->grid.rows.mem;
    counts.aieRows = profile->grid.rows.aie;

    DesignModel model;
    model.bundlePath = normalizedPath;
    model.name = manifest.value(QStringLiteral("name")).toString();
    if (model.name.trimmed().isEmpty())
        model.name = QFileInfo(normalizedPath).completeBaseName();
    model.deviceFamily = deviceFamily;
    model.aieArch = arch;
    model.deviceId = profile->id;
    model.tiles = counts;
    model.manifest = manifest;
    model.program = program;
    model.design = design;

    outModel = std::move(model);
    return Utils::Result::success();
}

Utils::Result DesignBundleLoader::resolveArchForDeviceFamily(const QString& deviceFamily, QString& outArch) const
{
    const QString token = normalizeToken(deviceFamily);
    if (token == kFamilyAieMl) {
        outArch = kArchAieMl;
        return Utils::Result::success();
    }

    if (token == kFamilyAieMlV2) {
        return Utils::Result::failure(QStringLiteral("Device family 'AIE-ML v2' is not supported yet."));
    }

    return Utils::Result::failure(QStringLiteral("Unknown device family: %1").arg(deviceFamily));
}

Utils::Result DesignBundleLoader::resolveProfileForArch(const QString& arch, const Aie::NpuProfile*& outProfile) const
{
    if (!m_catalog)
        return Utils::Result::failure(QStringLiteral("NPU profile catalog is not available."));

    const QString deviceId = QString::fromLatin1(Aie::kDefaultDeviceId);
    const Aie::NpuProfile* profile = findProfileById(*m_catalog, deviceId);
    if (!profile)
        return Utils::Result::failure(QStringLiteral("Device profile not found: %1").arg(deviceId));

    if (!profile->aieArch.isEmpty() && !archMatches(profile->aieArch, arch)) {
        return Utils::Result::failure(QStringLiteral("Device profile architecture '%1' does not match '%2'.")
                                          .arg(profile->aieArch, arch));
    }

    outProfile = profile;
    return Utils::Result::success();
}

bool DesignBundleLoader::archMatches(QStringView lhs, QStringView rhs)
{
    return QString::compare(lhs.toString().trimmed(), rhs.toString().trimmed(), Qt::CaseInsensitive) == 0;
}

} // namespace Aie::Internal
