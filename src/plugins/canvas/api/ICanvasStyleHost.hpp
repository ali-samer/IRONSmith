#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/api/CanvasStyleTypes.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Canvas::Api {

class CANVAS_EXPORT ICanvasStyleHost : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ICanvasStyleHost() override = default;

    virtual bool setBlockStyle(const QString& key, const CanvasBlockStyle& style) = 0;
    virtual bool clearBlockStyle(const QString& key) = 0;
    virtual void clearAll() = 0;

    virtual bool hasBlockStyle(const QString& key) const = 0;
    virtual CanvasBlockStyle blockStyle(const QString& key) const = 0;
    virtual QStringList blockStyleKeys() const = 0;

signals:
    void blockStyleChanged(const QString& key, const CanvasBlockStyle& style);
    void blockStyleRemoved(const QString& key);
    void blockStylesCleared();
};

} // namespace Canvas::Api
