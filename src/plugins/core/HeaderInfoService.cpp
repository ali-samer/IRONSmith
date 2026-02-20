// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "core/HeaderInfoService.hpp"

#include "core/StatusBarField.hpp"
#include "core/widgets/InfoBarWidget.hpp"

namespace Core::Internal {

namespace {

const QString kDefaultDevice = QStringLiteral("PHOENIX-XDNA1");
const QString kDefaultDesign = QStringLiteral("No design open");

} // namespace

HeaderInfoService::HeaderInfoService(QObject* parent)
    : Core::IHeaderInfo(parent)
    , m_deviceLabel(kDefaultDevice)
    , m_designLabel(kDefaultDesign)
{
}

void HeaderInfoService::bindInfoBar(InfoBarWidget* bar)
{
    if (m_bar == bar)
        return;
    m_bar = bar;
    ensureFields();
}

void HeaderInfoService::setDeviceLabel(QString label)
{
    label = label.trimmed();
    if (label.isEmpty())
        label = kDefaultDevice;
    if (m_deviceLabel == label)
        return;
    m_deviceLabel = label;
    if (m_deviceField)
        m_deviceField->setValue(m_deviceLabel);
    emit deviceLabelChanged(m_deviceLabel);
}

void HeaderInfoService::setDesignLabel(QString label)
{
    label = label.trimmed();
    if (label.isEmpty())
        label = kDefaultDesign;
    if (m_designLabel == label)
        return;
    m_designLabel = label;
    if (m_designField)
        m_designField->setValue(m_designLabel);
    emit designLabelChanged(m_designLabel);
}

void HeaderInfoService::ensureFields()
{
    if (!m_bar)
        return;

    m_deviceField = m_bar->ensureField(QStringLiteral("device"));
    if (m_deviceField) {
        m_deviceField->setLabel(QStringLiteral("DEVICE"));
        m_deviceField->setSide(Core::StatusBarField::Side::Left);
        m_deviceField->setValue(m_deviceLabel);
    }

    m_designField = m_bar->ensureField(QStringLiteral("design"));
    if (m_designField) {
        m_designField->setLabel(QStringLiteral("DESIGN"));
        m_designField->setSide(Core::StatusBarField::Side::Left);
        m_designField->setValue(m_designLabel);
    }
}

} // namespace Core::Internal
