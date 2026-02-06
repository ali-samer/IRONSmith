#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/Qt>

namespace Utils {

class UTILS_EXPORT PathPatternMatcher final
{
public:
    PathPatternMatcher();

    void setPatterns(const QStringList& patterns);
    QStringList patterns() const;

    void setCaseSensitivity(Qt::CaseSensitivity sensitivity);
    Qt::CaseSensitivity caseSensitivity() const;

    bool matches(const QString& relativePath, bool isDir) const;
    bool isEmpty() const;

    static QString normalizePath(const QString& path);

private:
    struct CompiledPattern final {
        QString source;
        QRegularExpression regex;
        bool directoryOnly = false;
        bool pathScoped = false;
    };

    QList<CompiledPattern> m_patterns;
    QStringList m_patternStrings;
    Qt::CaseSensitivity m_caseSensitivity = Qt::CaseInsensitive;
};

} // namespace Utils
