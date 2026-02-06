#include "utils/ui/LabeledSlider.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

namespace Utils {

LabeledSlider::LabeledSlider(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_slider = new QSlider(orientation, this);
    m_slider->setRange(0, 100);
    m_slider->setSingleStep(1);
    m_slider->setPageStep(4);

    m_valueLabel = new QLabel(QStringLiteral("0"), this);
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setMinimumWidth(36);

    layout->addWidget(m_slider, 1);
    layout->addWidget(m_valueLabel, 0);

    connect(m_slider, &QSlider::sliderPressed, this, &LabeledSlider::sliderPressed);
    connect(m_slider, &QSlider::sliderReleased, this, &LabeledSlider::sliderReleased);
    connect(m_slider, &QSlider::sliderMoved, this, &LabeledSlider::sliderMoved);
    connect(m_slider, &QSlider::valueChanged, this, [this](int value) {
        updateValueLabel(value);
        emit valueChanged(value);
    });
}

int LabeledSlider::value() const
{
    return m_slider->value();
}

void LabeledSlider::setValue(int value)
{
    m_slider->setValue(value);
    updateValueLabel(m_slider->value());
}

int LabeledSlider::minimum() const
{
    return m_slider->minimum();
}

void LabeledSlider::setMinimum(int min)
{
    m_slider->setMinimum(min);
    updateValueLabel(m_slider->value());
}

int LabeledSlider::maximum() const
{
    return m_slider->maximum();
}

void LabeledSlider::setMaximum(int max)
{
    m_slider->setMaximum(max);
    updateValueLabel(m_slider->value());
}

void LabeledSlider::setRange(int min, int max)
{
    m_slider->setRange(min, max);
    updateValueLabel(m_slider->value());
}

int LabeledSlider::singleStep() const
{
    return m_slider->singleStep();
}

void LabeledSlider::setSingleStep(int step)
{
    m_slider->setSingleStep(step);
}

int LabeledSlider::pageStep() const
{
    return m_slider->pageStep();
}

void LabeledSlider::setPageStep(int step)
{
    m_slider->setPageStep(step);
}

QString LabeledSlider::suffix() const
{
    return m_suffix;
}

void LabeledSlider::setSuffix(const QString& suffix)
{
    m_suffix = suffix;
    updateValueLabel(m_slider->value());
}

void LabeledSlider::setSpecialValue(int value, const QString& text)
{
    m_hasSpecialValue = true;
    m_specialValue = value;
    m_specialText = text;
    updateValueLabel(m_slider->value());
}

void LabeledSlider::clearSpecialValue()
{
    m_hasSpecialValue = false;
    m_specialText.clear();
    updateValueLabel(m_slider->value());
}

void LabeledSlider::updateValueLabel(int value)
{
    if (m_hasSpecialValue && value == m_specialValue) {
        m_valueLabel->setText(m_specialText);
        return;
    }
    m_valueLabel->setText(QString::number(value) + m_suffix);
}

} // namespace Utils
