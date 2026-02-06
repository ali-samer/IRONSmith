#include "canvas/internal/CanvasStyleHostImpl.hpp"

namespace Canvas::Internal {

bool CanvasStyleHostImpl::setBlockStyle(const QString& key, const Canvas::Api::CanvasBlockStyle& style)
{
    const QString trimmed = key.trimmed();
    if (trimmed.isEmpty())
        return false;

    m_styles.insert(trimmed, style);
    emit blockStyleChanged(trimmed, style);
    return true;
}

bool CanvasStyleHostImpl::clearBlockStyle(const QString& key)
{
    const QString trimmed = key.trimmed();
    if (trimmed.isEmpty())
        return false;

    const bool removed = m_styles.remove(trimmed) > 0;
    if (removed)
        emit blockStyleRemoved(trimmed);
    return removed;
}

void CanvasStyleHostImpl::clearAll()
{
    if (m_styles.isEmpty())
        return;
    m_styles.clear();
    emit blockStylesCleared();
}

bool CanvasStyleHostImpl::hasBlockStyle(const QString& key) const
{
    return m_styles.contains(key);
}

Canvas::Api::CanvasBlockStyle CanvasStyleHostImpl::blockStyle(const QString& key) const
{
    return m_styles.value(key, Canvas::Api::CanvasBlockStyle{});
}

QStringList CanvasStyleHostImpl::blockStyleKeys() const
{
    return m_styles.keys();
}

} // namespace Canvas::Internal
