#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Canvas {
class CanvasDocument;
class CanvasController;

namespace Api {
class CANVAS_EXPORT ICanvasHost : public QObject {
	Q_OBJECT

public:
	using QObject::QObject;
	~ICanvasHost() override = default;

	virtual QWidget* viewWidget() const = 0;

	virtual CanvasDocument*   document() const = 0;
	virtual CanvasController* controller() const = 0;

    virtual void setCanvasActive(bool active) = 0;
    virtual bool canvasActive() const = 0;
    virtual void setEmptyStateText(const QString& title, const QString& message) = 0;

signals:
    void canvasActiveChanged(bool active);
};
} // namespace Canvas::Api
} // namespace Canvas
