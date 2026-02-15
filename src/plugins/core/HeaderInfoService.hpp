// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "core/api/IHeaderInfo.hpp"

#include <QtCore/QPointer>

namespace Core {
class InfoBarWidget;
class StatusBarField;

namespace Internal {

class HeaderInfoService final : public Core::IHeaderInfo
{
    Q_OBJECT

public:
    explicit HeaderInfoService(QObject* parent = nullptr);

    void bindInfoBar(InfoBarWidget* bar);

    QString deviceLabel() const override { return m_deviceLabel; }
    QString designLabel() const override { return m_designLabel; }

public slots:
    void setDeviceLabel(QString label) override;
    void setDesignLabel(QString label) override;

private:
    void ensureFields();

    QPointer<InfoBarWidget> m_bar;
    QPointer<StatusBarField> m_deviceField;
    QPointer<StatusBarField> m_designField;
    QString m_deviceLabel;
    QString m_designLabel;
};

} // namespace Internal
} // namespace Core
