#include "utils/filesystem/FileSystemUtils.hpp"

#include <QtCore/QFileInfo>

namespace Utils::FileSystemUtils {

QString uniqueChildName(const QDir& dir, const QString& baseName, const QString& ext)
{
    const QString trimmedBase = baseName.trimmed();
    if (trimmedBase.isEmpty())
        return {};

    QString normalizedExt = ext.trimmed();
    if (normalizedExt.startsWith('.'))
        normalizedExt.remove(0, 1);

    const QString suffix = normalizedExt.isEmpty()
                               ? QString()
                               : QStringLiteral(".%1").arg(normalizedExt);
    const QString candidate = trimmedBase + suffix;
    if (!dir.exists(candidate))
        return candidate;

    for (int i = 1; i < 1000; ++i) {
        const QString indexed = QStringLiteral("%1 (%2)%3").arg(trimmedBase).arg(i).arg(suffix);
        if (!dir.exists(indexed))
            return indexed;
    }

    return {};
}

QString duplicateName(const QDir& dir, const QString& fileName)
{
    const QFileInfo fi(fileName);
    const QString base = fi.completeBaseName();
    const QString ext = fi.completeSuffix();
    if (base.trimmed().isEmpty())
        return {};

    const QString copyBase = QStringLiteral("%1 copy").arg(base);
    QString candidate = uniqueChildName(dir, copyBase, ext);
    if (!candidate.isEmpty())
        return candidate;

    return uniqueChildName(dir, base, ext);
}

} // namespace Utils::FileSystemUtils
