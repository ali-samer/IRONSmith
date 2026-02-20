// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/ui/BaseDialog.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace Utils {

BaseDialog::BaseDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("BaseDialog"));
    setAttribute(Qt::WA_StyledBackground, true);
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("DialogHeader"));
    auto* headerLayout = new QVBoxLayout(m_header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    m_titleLabel = new QLabel(m_header);
    m_titleLabel->setObjectName(QStringLiteral("DialogTitle"));
    QFont titleFont = m_titleLabel->font();
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setVisible(false);

    m_messageLabel = new QLabel(m_header);
    m_messageLabel->setObjectName(QStringLiteral("DialogMessage"));
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_messageLabel);
    root->addWidget(m_header);

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("DialogBody"));
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(10);
    root->addWidget(m_content, 1);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setObjectName(QStringLiteral("DialogButtons"));
    root->addWidget(m_buttonBox);

    updateHeader();
}

QString BaseDialog::titleText() const
{
    return m_titleText;
}

void BaseDialog::setTitleText(const QString& text)
{
    const QString cleaned = text.trimmed();
    if (m_titleText == cleaned)
        return;
    m_titleText = cleaned;
    setWindowTitle(m_titleText);
    updateHeader();
    emit titleTextChanged(m_titleText);
}

QString BaseDialog::messageText() const
{
    return m_messageText;
}

void BaseDialog::setMessageText(const QString& text)
{
    const QString cleaned = text.trimmed();
    if (m_messageText == cleaned)
        return;
    m_messageText = cleaned;
    updateHeader();
    emit messageTextChanged(m_messageText);
}

QLabel* BaseDialog::titleLabel() const
{
    return m_titleLabel;
}

QLabel* BaseDialog::messageLabel() const
{
    return m_messageLabel;
}

QWidget* BaseDialog::contentWidget() const
{
    return m_content;
}

QVBoxLayout* BaseDialog::contentLayout() const
{
    return m_contentLayout;
}

QDialogButtonBox* BaseDialog::buttonBox() const
{
    return m_buttonBox;
}

void BaseDialog::updateHeader()
{
    if (m_titleLabel) {
        m_titleLabel->setVisible(!m_titleText.isEmpty());
        m_titleLabel->setText(m_titleText);
    }

    if (m_messageLabel) {
        m_messageLabel->setVisible(!m_messageText.isEmpty());
        m_messageLabel->setText(m_messageText);
    }

    if (m_header)
        m_header->setVisible(!m_titleText.isEmpty() || !m_messageText.isEmpty());
}

} // namespace Utils
