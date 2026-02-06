#include "utils/VirtualPath.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace Utils {

VirtualPath::VirtualPath(QString path, Kind kind, bool isAbsolute)
    : m_path(std::move(path))
    , m_kind(kind)
    , m_isAbsolute(isAbsolute)
{
}

QString VirtualPath::normalize(QString path, Kind, bool* outAbsolute)
{
    path = QDir::fromNativeSeparators(path.trimmed());
    const bool abs = QDir::isAbsolutePath(path);

    QString cleaned = QDir::cleanPath(path);
    if (cleaned == ".")
        cleaned.clear();

    if (outAbsolute)
        *outAbsolute = abs || cleaned.startsWith('/');

    return cleaned;
}

VirtualPath VirtualPath::fromFileSystem(QString path)
{
    bool abs = false;
    const QString cleaned = normalize(std::move(path), Kind::FileSystem, &abs);
    return VirtualPath(cleaned, Kind::FileSystem, abs);
}

VirtualPath VirtualPath::fromBundle(QString path)
{
    bool abs = false;
    const QString cleaned = normalize(std::move(path), Kind::Bundle, &abs);
    return VirtualPath(cleaned, Kind::Bundle, abs);
}

QString VirtualPath::toNativeSeparators() const
{
    if (m_kind != Kind::FileSystem)
        return m_path;
    return QDir::toNativeSeparators(m_path);
}

QString VirtualPath::basename() const
{
    if (m_path.isEmpty())
        return {};
    const int slash = m_path.lastIndexOf('/');
    if (slash < 0)
        return m_path;
    if (slash == m_path.size() - 1)
        return {};
    return m_path.mid(slash + 1);
}

QString VirtualPath::extension() const
{
    const QString name = basename();
    const int dot = name.lastIndexOf('.');
    if (dot <= 0)
        return {};
    return name.mid(dot + 1);
}

QString VirtualPath::stem() const
{
    const QString name = basename();
    const int dot = name.lastIndexOf('.');
    if (dot <= 0)
        return name;
    return name.left(dot);
}

QStringList VirtualPath::segments() const
{
    if (m_path.isEmpty())
        return {};
    return m_path.split('/', Qt::SkipEmptyParts);
}

VirtualPath VirtualPath::parent() const
{
    if (m_path.isEmpty())
        return {};
    if (m_path == QStringLiteral("/"))
        return *this;

    const int slash = m_path.lastIndexOf('/');
    if (slash < 0)
        return VirtualPath(QString(), m_kind, false);
    if (slash == 0)
        return VirtualPath(QStringLiteral("/"), m_kind, true);

    return VirtualPath(m_path.left(slash), m_kind, m_isAbsolute);
}

VirtualPath VirtualPath::join(QStringView child) const
{
    const QString childStr = child.toString();
    if (childStr.trimmed().isEmpty())
        return *this;

    if (QDir::isAbsolutePath(childStr))
        return (m_kind == Kind::Bundle) ? fromBundle(childStr) : fromFileSystem(childStr);

    if (m_path.isEmpty())
        return (m_kind == Kind::Bundle) ? fromBundle(childStr) : fromFileSystem(childStr);

    QString joined = m_path;
    if (!joined.endsWith('/'))
        joined.append('/');
    joined.append(childStr);

    return (m_kind == Kind::Bundle) ? fromBundle(joined) : fromFileSystem(joined);
}

bool VirtualPath::startsWith(const VirtualPath& other) const
{
    if (m_kind != other.m_kind)
        return false;
    if (other.m_isAbsolute != m_isAbsolute)
        return false;
    if (other.m_path.isEmpty())
        return true;

    const QStringList lhs = segments();
    const QStringList rhs = other.segments();
    if (rhs.size() > lhs.size())
        return false;

    for (int i = 0; i < rhs.size(); ++i) {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

std::strong_ordering operator<=>(const VirtualPath& a, const VirtualPath& b) noexcept
{
    if (a.kind() != b.kind()) {
        const int ak = static_cast<int>(a.kind());
        const int bk = static_cast<int>(b.kind());
        return ak < bk ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    if (a.isAbsolute() != b.isAbsolute())
        return a.isAbsolute() ? std::strong_ordering::greater : std::strong_ordering::less;

    const int c = QString::compare(a.path(), b.path(), Qt::CaseSensitive);
    if (c < 0) return std::strong_ordering::less;
    if (c > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

size_t qHash(const VirtualPath& path, size_t seed) noexcept
{
    seed = ::qHash(static_cast<int>(path.kind()), seed);
    seed = ::qHash(path.isAbsolute(), seed);
    return ::qHash(path.path(), seed);
}

} // namespace Utils
