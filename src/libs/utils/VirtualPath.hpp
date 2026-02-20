// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>

#include <compare>

namespace Utils {

class UTILS_EXPORT VirtualPath final {
public:
    enum class Kind {
        FileSystem,
        Bundle
    };

    VirtualPath() = default;

    static VirtualPath fromFileSystem(QString path);
    static VirtualPath fromBundle(QString path);

    bool isEmpty() const noexcept { return m_path.isEmpty(); }
    bool isAbsolute() const noexcept { return m_isAbsolute; }
    bool isRelative() const noexcept { return !m_isAbsolute; }
    Kind kind() const noexcept { return m_kind; }

    const QString& path() const noexcept { return m_path; }
    QString toString() const { return m_path; }
    QString toNativeSeparators() const;

    QString basename() const;
    QString stem() const;
    QString extension() const;
    QStringList segments() const;

    VirtualPath parent() const;
    VirtualPath join(QStringView child) const;
    bool startsWith(const VirtualPath& other) const;

    friend bool operator==(const VirtualPath& a, const VirtualPath& b) noexcept
    {
        return a.m_kind == b.m_kind && a.m_isAbsolute == b.m_isAbsolute && a.m_path == b.m_path;
    }

    friend std::strong_ordering operator<=>(const VirtualPath& a, const VirtualPath& b) noexcept;

private:
    VirtualPath(QString path, Kind kind, bool isAbsolute);
    static QString normalize(QString path, Kind kind, bool* outAbsolute);

    QString m_path;
    Kind m_kind = Kind::FileSystem;
    bool m_isAbsolute = false;
};

UTILS_EXPORT size_t qHash(const VirtualPath& path, size_t seed = 0) noexcept;

} // namespace Utils
