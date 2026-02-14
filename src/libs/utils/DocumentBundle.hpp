#pragma once

#include "utils/Result.hpp"
#include "utils/UtilsGlobal.hpp"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Utils {

class UTILS_EXPORT DocumentBundle final
{
public:
    struct BundleInit final {
        QString name;
        QString documentId;
        QDateTime createdAtUtc;
        QDateTime modifiedAtUtc;

        QJsonObject program;
        QJsonObject design;

        QJsonArray assets;
        QStringList tags;
        QString notes;
        QString thumbnailPath;
    };

    struct BundleInfo final {
        QString path;
        QString name;
        QString documentId;
        bool valid = false;
        QString error;
        QJsonObject manifest;
    };

    static QString extension();
    static QString manifestFileName();
    static QString programFileName();
    static QString designFileName();
    static QString defaultIconResource();

    static QString normalizeBundlePath(QStringView path);
    static bool hasBundleExtension(QStringView path);
    static bool isBundlePath(QStringView path);
    static bool isBundle(const QString& path, QString* error = nullptr);

    static Result create(const QString& path, const BundleInit& init);
    static Result validate(const QString& path, QString* error = nullptr);
    static BundleInfo probe(const QString& path);

    static Result writeProgram(const QString& path, const QJsonObject& program);
    static Result writeDesign(const QString& path, const QJsonObject& design);
    static Result writeManifest(const QString& path, const QJsonObject& manifest);

    static QJsonObject readProgram(const QString& path, QString* error = nullptr);
    static QJsonObject readDesign(const QString& path, QString* error = nullptr);
    static QJsonObject readManifest(const QString& path, QString* error = nullptr);
};

} // namespace Utils
