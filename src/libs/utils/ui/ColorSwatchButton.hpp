#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtGui/QColor>
#include <QtWidgets/QToolButton>

namespace Utils {

class UTILS_EXPORT ColorSwatchButton final : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(bool allowAlpha READ allowAlpha WRITE setAllowAlpha NOTIFY allowAlphaChanged)

public:
    explicit ColorSwatchButton(QWidget* parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor& color);

    bool allowAlpha() const { return m_allowAlpha; }
    void setAllowAlpha(bool allow);

signals:
    void colorChanged(const QColor& color);
    void allowAlphaChanged(bool allow);

private slots:
    void pickColor();

private:
    void updateSwatch();

    QColor m_color;
    bool m_allowAlpha = false;
};

} // namespace Utils
