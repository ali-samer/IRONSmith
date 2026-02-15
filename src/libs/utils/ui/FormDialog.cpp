// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/ui/FormDialog.hpp"

#include <QtWidgets/QFormLayout>

namespace Utils {

FormDialog::FormDialog(QWidget* parent)
    : BaseDialog(parent)
{
    m_formLayout = new QFormLayout();
    m_formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_formLayout->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_formLayout->setHorizontalSpacing(12);
    m_formLayout->setVerticalSpacing(10);
    contentLayout()->addLayout(m_formLayout);
}

QFormLayout* FormDialog::formLayout() const
{
    return m_formLayout;
}

} // namespace Utils
