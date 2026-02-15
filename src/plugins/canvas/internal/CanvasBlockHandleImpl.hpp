// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/api/ICanvasBlockHandle.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointer>

namespace Canvas {
class CanvasDocument;
class CanvasBlock;
}

namespace Canvas::Internal {

class CanvasBlockHandleImpl final : public Canvas::Api::ICanvasBlockHandle
{
    Q_OBJECT

public:
    explicit CanvasBlockHandleImpl(QString id,
                                   CanvasDocument* document,
                                   CanvasBlock* block,
                                   QObject* parent = nullptr);

    QString id() const override;

    void setLabel(const QString& label) override;
    void setMovable(bool movable) override;
    void setShowPorts(bool show) override;
    void setKeepoutMargin(double marginScene) override;
    void setContentPadding(const QMarginsF& padding) override;

    void setContent(std::unique_ptr<Canvas::BlockContent> content) override;
    Canvas::BlockContent* content() const override;

    CanvasBlock* block() const;
    void setBlock(CanvasBlock* block);

private:
    QString m_id;
    QPointer<CanvasDocument> m_document;
    ObjectId m_blockId{};
};

} // namespace Canvas::Internal
