#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtWidgets/QWidget>

class QSlider;
class QLabel;

namespace Utils {

class UTILS_EXPORT LabeledSlider final : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)

public:
    explicit LabeledSlider(Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    int singleStep() const;
    void setSingleStep(int step);
    int pageStep() const;
    void setPageStep(int step);

    QString suffix() const;
    void setSuffix(const QString& suffix);

    void setSpecialValue(int value, const QString& text);
    void clearSpecialValue();

signals:
    void valueChanged(int value);
    void sliderPressed();
    void sliderReleased();
    void sliderMoved(int value);

private:
    void updateValueLabel(int value);

    QSlider* m_slider = nullptr;
    QLabel* m_valueLabel = nullptr;
    QString m_suffix;
    bool m_hasSpecialValue = false;
    int m_specialValue = 0;
    QString m_specialText;
};

} // namespace Utils
