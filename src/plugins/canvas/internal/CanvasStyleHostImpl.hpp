#pragma once

#include "canvas/api/ICanvasStyleHost.hpp"

#include <QtCore/QHash>

namespace Canvas::Internal {

class CanvasStyleHostImpl final : public Canvas::Api::ICanvasStyleHost
{
    Q_OBJECT

public:
    using Canvas::Api::ICanvasStyleHost::ICanvasStyleHost;

    bool setBlockStyle(const QString& key, const Canvas::Api::CanvasBlockStyle& style) override;
    bool clearBlockStyle(const QString& key) override;
    void clearAll() override;

    bool hasBlockStyle(const QString& key) const override;
    Canvas::Api::CanvasBlockStyle blockStyle(const QString& key) const override;
    QStringList blockStyleKeys() const override;

private:
    QHash<QString, Canvas::Api::CanvasBlockStyle> m_styles;
};

} // namespace Canvas::Internal
