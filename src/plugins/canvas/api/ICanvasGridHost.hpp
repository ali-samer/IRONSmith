// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/api/CanvasGridTypes.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

namespace Utils {
struct GridSpec;
}

namespace Canvas::Api {
class ICanvasBlockHandle;

class CANVAS_EXPORT ICanvasGridHost : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~ICanvasGridHost() override = default;

    virtual void setGridSpec(const Utils::GridSpec& spec) = 0;
    virtual Utils::GridSpec gridSpec() const = 0;

    virtual void setBlocks(const QVector<CanvasBlockSpec>& blocks) = 0;
    virtual void clearBlocks() = 0;

    virtual ICanvasBlockHandle* blockHandle(const QString& id) const = 0;
    virtual QVector<ICanvasBlockHandle*> blockHandles() const = 0;

signals:
    void gridSpecChanged(const Utils::GridSpec& spec);
    void blocksChanged();
};

} // namespace Canvas::Api
