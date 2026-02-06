#include "utils/ui/ColorSwatchButton.hpp"

#include <QtWidgets/QColorDialog>

namespace Utils {

ColorSwatchButton::ColorSwatchButton(QWidget* parent)
    : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAutoRaise(true);
    setFixedSize(28, 20);
    connect(this, &QToolButton::clicked, this, &ColorSwatchButton::pickColor);
    updateSwatch();
}

void ColorSwatchButton::setColor(const QColor& color)
{
    if (m_color == color)
        return;
    m_color = color;
    updateSwatch();
    emit colorChanged(m_color);
}

void ColorSwatchButton::setAllowAlpha(bool allow)
{
    if (m_allowAlpha == allow)
        return;
    m_allowAlpha = allow;
    emit allowAlphaChanged(m_allowAlpha);
}

void ColorSwatchButton::pickColor()
{
    QColorDialog dialog(m_color, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel, m_allowAlpha);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QColor chosen = dialog.selectedColor();
    if (!chosen.isValid())
        return;

    setColor(chosen);
}

void ColorSwatchButton::updateSwatch()
{
    const QColor color = m_color.isValid() ? m_color : QColor(Qt::transparent);
    const QString style = QStringLiteral(
        "QToolButton { background-color: %1; border: 1px solid rgba(255,255,255,40); border-radius: 3px; }")
            .arg(color.name(QColor::HexArgb));
    setStyleSheet(style);
}

} // namespace Utils
