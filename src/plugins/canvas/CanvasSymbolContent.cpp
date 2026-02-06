#include "canvas/CanvasSymbolContent.hpp"

#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>

namespace Canvas {

namespace {

QFont symbolFont(QPainter& p, const SymbolContentStyle& style)
{
    QFont f = p.font();
    f.setPointSizeF(style.pointSize);
    f.setBold(style.bold);
    return f;
}

QSizeF symbolSize(const QString& symbol, const SymbolContentStyle& style)
{
    if (symbol.isEmpty())
        return {};
    QFont f;
    f.setPointSizeF(style.pointSize);
    f.setBold(style.bold);
    QFontMetricsF metrics(f);
    return metrics.size(Qt::TextSingleLine, symbol);
}

} // namespace

BlockContentSymbol::BlockContentSymbol(QString symbol, SymbolContentStyle style)
    : m_symbol(std::move(symbol))
    , m_style(std::move(style))
{}

QSizeF BlockContentSymbol::measure(const CanvasRenderContext& ctx) const
{
    Q_UNUSED(ctx);
    if (m_hasPreferredSize)
        return m_preferredSize;
    return symbolSize(m_symbol, m_style);
}

void BlockContentSymbol::draw(QPainter& p, const CanvasRenderContext& ctx) const
{
    Q_UNUSED(ctx);
    if (m_symbol.isEmpty())
        return;

    QFont f = symbolFont(p, m_style);
    p.setFont(f);
    p.setPen(m_style.text);
    p.drawText(m_bounds, Qt::AlignCenter, m_symbol);
}

std::unique_ptr<BlockContent> BlockContentSymbol::clone() const
{
    auto copy = std::make_unique<BlockContentSymbol>(m_symbol, m_style);
    if (m_hasPreferredSize)
        copy->setPreferredSize(m_preferredSize);
    return copy;
}

} // namespace Canvas
