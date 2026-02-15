// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/design/DesignBundleCreator.hpp"

#include <utils/DocumentBundle.hpp>
#include <utils/filesystem/FileSystemUtils.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>

namespace Aie::Internal {

Utils::Result DesignBundleCreator::validateRequest(const DesignBundleCreateRequest& request)
{
    const QString name = request.name.trimmed();
    if (name.isEmpty())
        return Utils::Result::failure(QStringLiteral("Name cannot be empty."));

    if (containsPathSeparators(name))
        return Utils::Result::failure(QStringLiteral("Name cannot contain path separators."));

    const QString location = request.location.trimmed();
    if (location.isEmpty())
        return Utils::Result::failure(QStringLiteral("Location cannot be empty."));

    const QString deviceFamily = request.deviceFamily.trimmed();
    if (deviceFamily.isEmpty())
        return Utils::Result::failure(QStringLiteral("Device family cannot be empty."));

    return Utils::Result::success();
}

QString DesignBundleCreator::resolveBundlePath(const QString& location, const QString& name)
{
    const QString trimmedName = name.trimmed();
    const QString trimmedLocation = location.trimmed();
    if (trimmedName.isEmpty() || trimmedLocation.isEmpty())
        return {};

    const QString candidate = QDir(trimmedLocation).filePath(trimmedName);
    return Utils::DocumentBundle::normalizeBundlePath(candidate);
}

Utils::Result DesignBundleCreator::create(const DesignBundleCreateRequest& request,
                                          ExistingBundlePolicy policy,
                                          DesignBundleCreateResult& outResult)
{
    outResult = DesignBundleCreateResult{};

    const Utils::Result valid = validateRequest(request);
    if (!valid)
        return valid;

    const Utils::Result ensureLocation = ensureLocationExists(request.location.trimmed());
    if (!ensureLocation)
        return ensureLocation;

    QString bundlePath = resolveBundlePath(request.location, request.name);
    if (bundlePath.isEmpty())
        return Utils::Result::failure(QStringLiteral("Unable to resolve bundle path."));

    const bool exists = QFileInfo::exists(bundlePath);
    if (exists) {
        switch (policy) {
            case ExistingBundlePolicy::FailIfExists:
                return Utils::Result::failure(QStringLiteral("A design already exists at this location."));
            case ExistingBundlePolicy::ReplaceExisting: {
                const Utils::Result removed = removeExistingBundle(bundlePath);
                if (!removed)
                    return removed;
                outResult.replacedExisting = true;
                break;
            }
            case ExistingBundlePolicy::CreateCopy: {
                const QString uniquePath = uniqueBundlePath(bundlePath);
                if (uniquePath.isEmpty()) {
                    return Utils::Result::failure(
                        QStringLiteral("Unable to generate a unique design name."));
                }
                bundlePath = uniquePath;
                outResult.createdCopy = true;
                break;
            }
        }
    }

    Utils::DocumentBundle::BundleInit init;
    init.name = QFileInfo(bundlePath).completeBaseName();
    init.program = QJsonObject{
        { QStringLiteral("deviceFamily"), request.deviceFamily.trimmed() }
    };
    init.design = QJsonObject{};

    const Utils::Result created = Utils::DocumentBundle::create(bundlePath, init);
    if (!created)
        return created;

    outResult.bundlePath = bundlePath;
    outResult.displayName = init.name;
    return Utils::Result::success();
}

Utils::Result DesignBundleCreator::ensureLocationExists(const QString& location)
{
    const QString trimmed = location.trimmed();
    if (trimmed.isEmpty())
        return Utils::Result::failure(QStringLiteral("Location cannot be empty."));

    QDir dir(trimmed);
    if (dir.exists())
        return Utils::Result::success();

    if (!dir.mkpath(QStringLiteral(".")))
        return Utils::Result::failure(QStringLiteral("Failed to create folder: %1").arg(trimmed));

    return Utils::Result::success();
}

Utils::Result DesignBundleCreator::removeExistingBundle(const QString& bundlePath)
{
    const QFileInfo existing(bundlePath);
    bool removed = false;
    if (existing.isDir())
        removed = QDir(bundlePath).removeRecursively();
    else
        removed = QFile::remove(bundlePath);

    if (!removed)
        return Utils::Result::failure(QStringLiteral("Failed to replace existing design."));

    return Utils::Result::success();
}

QString DesignBundleCreator::uniqueBundlePath(const QString& existingPath)
{
    const QFileInfo info(existingPath);
    const QDir dir(info.absolutePath());
    const QString candidate = Utils::FileSystemUtils::duplicateName(dir, info.fileName());
    if (candidate.isEmpty())
        return {};
    return dir.filePath(candidate);
}

bool DesignBundleCreator::containsPathSeparators(const QString& text)
{
    return text.contains(u'/') || text.contains(u'\\');
}

} // namespace Aie::Internal
