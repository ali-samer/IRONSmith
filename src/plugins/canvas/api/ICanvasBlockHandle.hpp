#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QMarginsF>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

namespace Canvas {
class BlockContent;
}

namespace Canvas::Api {

class CANVAS_EXPORT ICanvasBlockHandle : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ICanvasBlockHandle() override = default;

    virtual QString id() const = 0;

    virtual void setLabel(const QString& label) = 0;
    virtual void setMovable(bool movable) = 0;
    virtual void setShowPorts(bool show) = 0;
    virtual void setKeepoutMargin(double marginScene) = 0;
    virtual void setContentPadding(const QMarginsF& padding) = 0;

    virtual void setContent(std::unique_ptr<Canvas::BlockContent> content) = 0;
    virtual Canvas::BlockContent* content() const = 0;

signals:
    void labelChanged(const QString& label);
    void movableChanged(bool movable);
    void showPortsChanged(bool show);
    void keepoutMarginChanged(double marginScene);
    void contentPaddingChanged(const QMarginsF& padding);
    void contentChanged();
};

} // namespace Canvas::Api
