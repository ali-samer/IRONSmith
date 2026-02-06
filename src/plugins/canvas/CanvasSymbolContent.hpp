#pragma once

#include "canvas/CanvasBlockContent.hpp"

#include <QtGui/QColor>

namespace Canvas {

struct CANVAS_EXPORT SymbolContentStyle final {
    QColor text = QColor(Constants::kBlockTextColor);
    double pointSize = Constants::kBlockLabelPointSize;
    bool bold = true;
};

class CANVAS_EXPORT BlockContentSymbol final : public BlockContent
{
public:
    explicit BlockContentSymbol(QString symbol = {}, SymbolContentStyle style = {});

    const QString& symbol() const { return m_symbol; }
    void setSymbol(QString symbol) { m_symbol = std::move(symbol); }

    const SymbolContentStyle& style() const { return m_style; }
    void setStyle(SymbolContentStyle style) { m_style = std::move(style); }

    QSizeF measure(const CanvasRenderContext& ctx) const override;
    void draw(QPainter& p, const CanvasRenderContext& ctx) const override;
    std::unique_ptr<BlockContent> clone() const override;

private:
    QString m_symbol;
    SymbolContentStyle m_style;
};

} // namespace Canvas
