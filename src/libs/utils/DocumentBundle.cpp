// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/DocumentBundle.hpp"

#include "utils/PathUtils.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QSaveFile>
#include <QtCore/QUuid>

namespace Utils {

namespace {

using namespace Qt::StringLiterals;

const QString kExtension = u"ironsmith"_s;
const QString kManifestFile = u"manifest.json"_s;
const QString kProgramFile = u"program.json"_s;
const QString kDesignFile = u"design.json"_s;
const QString kDefaultIcon = u":/ui/icons/svg/hammer_icon.svg"_s;

QString isoUtc(const QDateTime& dt)
{
    if (!dt.isValid())
        return {};
    return dt.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime parseIsoUtc(const QString& text)
{
    const QDateTime dt = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (dt.isValid())
        return dt.toUTC();
    return {};
}

QString joinPath(const QString& base, const QString& file)
{
    QDir dir(base);
    return dir.filePath(file);
}

Result ensureDirectory(const QString& path)
{
    QDir dir;
    if (dir.exists(path))
        return Result::success();
    if (!dir.mkpath(path))
        return Result::failure(QStringLiteral("Failed to create bundle directory."));
    return Result::success();
}

Result writeJsonAtomic(const QString& path, const QJsonObject& obj)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return Result::failure(QStringLiteral("Failed to open file for writing: %1").arg(path));

    const QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Compact));
    if (!file.commit())
        return Result::failure(QStringLiteral("Failed to commit file: %1").arg(path));

    return Result::success();
}

QJsonObject readJsonFile(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Failed to open file: %1").arg(path);
        return {};
    }

    const QByteArray bytes = file.readAll();
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("Invalid JSON: %1").arg(path);
        return {};
    }
    return doc.object();
}

QJsonArray stringListToJson(const QStringList& list)
{
    QJsonArray arr;
    for (const auto& v : list)
        arr.push_back(v);
    return arr;
}

bool requiredFilesExist(const QString& path, QString* error)
{
    const QStringList files{ kManifestFile, kProgramFile, kDesignFile };
    for (const auto& file : files) {
        const QString full = joinPath(path, file);
        if (!QFileInfo::exists(full)) {
            if (error)
                *error = QStringLiteral("Missing required file: %1").arg(file);
            return false;
        }
    }
    return true;
}

} // namespace

QString DocumentBundle::extension()
{
    return kExtension;
}

QString DocumentBundle::manifestFileName()
{
    return kManifestFile;
}

QString DocumentBundle::programFileName()
{
    return kProgramFile;
}

QString DocumentBundle::designFileName()
{
    return kDesignFile;
}

QString DocumentBundle::defaultIconResource()
{
    return kDefaultIcon;
}

QString DocumentBundle::normalizeBundlePath(QStringView path)
{
    const QString normalized = PathUtils::normalizePath(path);
    return PathUtils::ensureExtension(normalized, kExtension);
}

bool DocumentBundle::hasBundleExtension(QStringView path)
{
    return PathUtils::hasExtension(path, kExtension);
}

bool DocumentBundle::isBundlePath(QStringView path)
{
    if (!hasBundleExtension(path))
        return false;
    QFileInfo info(path.toString());
    return info.exists() && info.isDir();
}

bool DocumentBundle::isBundle(const QString& path, QString* error)
{
    const QString normalized = normalizeBundlePath(path);
    QFileInfo info(normalized);
    if (!info.exists() || !info.isDir()) {
        if (error)
            *error = QStringLiteral("Bundle path is not a directory.");
        return false;
    }
    if (!hasBundleExtension(normalized)) {
        if (error)
            *error = QStringLiteral("Bundle extension mismatch.");
        return false;
    }
    return requiredFilesExist(normalized, error);
}

Result DocumentBundle::create(const QString& path, const BundleInit& init)
{
    const QString bundlePath = normalizeBundlePath(path);
    QFileInfo info(bundlePath);
    if (info.exists() && !info.isDir())
        return Result::failure(QStringLiteral("Bundle path exists and is not a directory."));

    if (info.exists() && info.isDir()) {
        const QDir dir(bundlePath);
        if (!dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
            return Result::failure(QStringLiteral("Bundle directory is not empty."));
    }

    const Result ensure = ensureDirectory(bundlePath);
    if (!ensure)
        return ensure;

    const QString programPath = joinPath(bundlePath, kProgramFile);
    const QString designPath = joinPath(bundlePath, kDesignFile);
    const QString manifestPath = joinPath(bundlePath, kManifestFile);

    Result writeProgramResult = writeJsonAtomic(programPath, init.program);
    if (!writeProgramResult)
        return writeProgramResult;
    Result writeDesignResult = writeJsonAtomic(designPath, init.design);
    if (!writeDesignResult)
        return writeDesignResult;

    const QString docId = init.documentId.isEmpty()
                              ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                              : init.documentId;
    const QDateTime createdAt = init.createdAtUtc.isValid()
                                    ? init.createdAtUtc.toUTC()
                                    : QDateTime::currentDateTimeUtc();
    const QDateTime modifiedAt = init.modifiedAtUtc.isValid()
                                     ? init.modifiedAtUtc.toUTC()
                                     : createdAt;

    QJsonObject manifest;
    manifest.insert(u"documentId"_s, docId);
    manifest.insert(u"name"_s, init.name);
    manifest.insert(u"createdAt"_s, isoUtc(createdAt));
    manifest.insert(u"modifiedAt"_s, isoUtc(modifiedAt));
    manifest.insert(u"files"_s, QJsonArray{ kProgramFile, kDesignFile });
    manifest.insert(u"icon"_s, kDefaultIcon);

    if (!init.assets.isEmpty())
        manifest.insert(u"assets"_s, init.assets);
    if (!init.tags.isEmpty())
        manifest.insert(u"tags"_s, stringListToJson(init.tags));
    if (!init.notes.isEmpty())
        manifest.insert(u"notes"_s, init.notes);
    if (!init.thumbnailPath.isEmpty())
        manifest.insert(u"thumbnail"_s, init.thumbnailPath);

    Result writeManifestResult = writeJsonAtomic(manifestPath, manifest);
    if (!writeManifestResult)
        return writeManifestResult;

    return Result::success();
}

Result DocumentBundle::validate(const QString& path, QString* error)
{
    if (!isBundle(path, error))
        return Result::failure(error ? *error : QStringLiteral("Bundle validation failed."));

    const QString manifestPath = joinPath(normalizeBundlePath(path), kManifestFile);
    QString err;
    const QJsonObject manifest = readJsonFile(manifestPath, &err);
    if (manifest.isEmpty()) {
        if (error)
            *error = err.isEmpty() ? QStringLiteral("Manifest is empty.") : err;
        return Result::failure(error ? *error : QStringLiteral("Manifest is empty."));
    }

    if (!manifest.contains(u"documentId"_s) || !manifest.contains(u"name"_s)) {
        if (error)
            *error = QStringLiteral("Manifest missing required fields.");
        return Result::failure(error ? *error : QStringLiteral("Manifest missing required fields."));
    }

    return Result::success();
}

DocumentBundle::BundleInfo DocumentBundle::probe(const QString& path)
{
    BundleInfo info;
    info.path = normalizeBundlePath(path);

    QString err;
    info.valid = isBundle(info.path, &err);
    if (!info.valid) {
        info.error = err;
        return info;
    }

    QString readErr;
    const QJsonObject manifest = readJsonFile(joinPath(info.path, kManifestFile), &readErr);
    info.manifest = manifest;
    info.documentId = manifest.value(u"documentId"_s).toString();
    info.name = manifest.value(u"name"_s).toString();
    if (!readErr.isEmpty()) {
        info.error = readErr;
        info.valid = false;
    }
    return info;
}

Result DocumentBundle::writeProgram(const QString& path, const QJsonObject& program)
{
    const QString target = joinPath(normalizeBundlePath(path), kProgramFile);
    return writeJsonAtomic(target, program);
}

Result DocumentBundle::writeDesign(const QString& path, const QJsonObject& design)
{
    const QString target = joinPath(normalizeBundlePath(path), kDesignFile);
    return writeJsonAtomic(target, design);
}

Result DocumentBundle::writeManifest(const QString& path, const QJsonObject& manifest)
{
    const QString target = joinPath(normalizeBundlePath(path), kManifestFile);
    return writeJsonAtomic(target, manifest);
}

QJsonObject DocumentBundle::readProgram(const QString& path, QString* error)
{
    return readJsonFile(joinPath(normalizeBundlePath(path), kProgramFile), error);
}

QJsonObject DocumentBundle::readDesign(const QString& path, QString* error)
{
    return readJsonFile(joinPath(normalizeBundlePath(path), kDesignFile), error);
}

QJsonObject DocumentBundle::readManifest(const QString& path, QString* error)
{
    return readJsonFile(joinPath(normalizeBundlePath(path), kManifestFile), error);
}

} // namespace Utils
