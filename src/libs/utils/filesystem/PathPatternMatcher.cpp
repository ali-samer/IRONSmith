// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/filesystem/PathPatternMatcher.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace Utils {

PathPatternMatcher::PathPatternMatcher() = default;

void PathPatternMatcher::setPatterns(const QStringList& patterns)
{
    m_patterns.clear();
    m_patternStrings.clear();

    for (const QString& raw : patterns) {
        QString pattern = raw.trimmed();
        if (pattern.isEmpty())
            continue;

        pattern = normalizePath(pattern);
        if (pattern.startsWith("./"))
            pattern.remove(0, 2);
        if (pattern.startsWith("/"))
            pattern.remove(0, 1);

        bool directoryOnly = false;
        if (pattern.endsWith('/')) {
            directoryOnly = true;
            pattern.chop(1);
        }

        if (pattern.isEmpty())
            continue;

        const bool pathScoped = pattern.contains('/');

        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (m_caseSensitivity == Qt::CaseInsensitive)
            options |= QRegularExpression::CaseInsensitiveOption;

        const QString regexString = QRegularExpression::wildcardToRegularExpression(pattern);
        QRegularExpression regex(regexString, options);

        CompiledPattern compiled;
        compiled.source = pattern;
        compiled.regex = regex;
        compiled.directoryOnly = directoryOnly;
        compiled.pathScoped = pathScoped;
        m_patterns.push_back(compiled);
        m_patternStrings.push_back(pattern);
    }
}

QStringList PathPatternMatcher::patterns() const
{
    return m_patternStrings;
}

void PathPatternMatcher::setCaseSensitivity(Qt::CaseSensitivity sensitivity)
{
    if (m_caseSensitivity == sensitivity)
        return;
    m_caseSensitivity = sensitivity;
    setPatterns(m_patternStrings);
}

Qt::CaseSensitivity PathPatternMatcher::caseSensitivity() const
{
    return m_caseSensitivity;
}

bool PathPatternMatcher::matches(const QString& relativePath, bool isDir) const
{
    if (m_patterns.isEmpty())
        return false;

    const QString normalized = normalizePath(relativePath);
    const QString baseName = QFileInfo(normalized).fileName();

    for (const auto& pattern : m_patterns) {
        if (pattern.directoryOnly && !isDir)
            continue;

        const QString subject = pattern.pathScoped ? normalized : baseName;
        if (pattern.regex.match(subject).hasMatch())
            return true;
    }

    return false;
}

bool PathPatternMatcher::isEmpty() const
{
    return m_patterns.isEmpty();
}

QString PathPatternMatcher::normalizePath(const QString& path)
{
    QString cleaned = QDir::fromNativeSeparators(path.trimmed());
    if (cleaned.startsWith("./"))
        cleaned.remove(0, 2);
    return cleaned;
}

} // namespace Utils
