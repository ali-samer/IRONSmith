#include "utils/ui/ConfirmationDialog.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

namespace Utils {

ConfirmationDialog::ConfirmationDialog(QWidget* parent)
    : BaseDialog(parent)
{
    m_informativeLabel = new QLabel(contentWidget());
    m_informativeLabel->setObjectName(QStringLiteral("DialogInformative"));
    m_informativeLabel->setWordWrap(true);
    m_informativeLabel->setVisible(false);

    m_detailsLabel = new QLabel(contentWidget());
    m_detailsLabel->setObjectName(QStringLiteral("DialogDetails"));
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setVisible(false);

    contentLayout()->addWidget(m_informativeLabel);
    contentLayout()->addWidget(m_detailsLabel);

    auto* buttons = buttonBox();
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_confirmButton = buttons->button(QDialogButtonBox::Ok);
    m_cancelButton = buttons->button(QDialogButtonBox::Cancel);
    if (m_confirmButton)
        m_confirmButton->setObjectName(QStringLiteral("DialogConfirmButton"));
    if (m_cancelButton)
        m_cancelButton->setObjectName(QStringLiteral("DialogCancelButton"));

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateButtons();
}

bool ConfirmationDialog::confirm(QWidget* parent, const ConfirmationDialogConfig& config)
{
    ConfirmationDialog dialog(parent);
    dialog.setTitle(config.title);
    dialog.setMessage(config.message);
    dialog.setInformativeText(config.informativeText);
    dialog.setDetails(config.details);
    dialog.setDestructive(config.destructive);
    dialog.setConfirmButtonText(config.confirmText);
    dialog.setCancelButtonText(config.cancelText);
    return dialog.exec() == QDialog::Accepted;
}

bool ConfirmationDialog::confirmDelete(QWidget* parent, const QString& targetName, bool isFolder)
{
    ConfirmationDialogConfig cfg;
    cfg.title = isFolder ? QStringLiteral("Delete Folder") : QStringLiteral("Delete File");
    cfg.message = isFolder
        ? QStringLiteral("Delete '%1' and all of its contents?").arg(targetName)
        : QStringLiteral("Delete '%1'?").arg(targetName);
    cfg.confirmText = QStringLiteral("Delete");
    cfg.cancelText = QStringLiteral("Cancel");
    cfg.destructive = true;
    return confirm(parent, cfg);
}

QString ConfirmationDialog::title() const
{
    return titleText();
}

void ConfirmationDialog::setTitle(const QString& title)
{
    const QString cleaned = title.trimmed();
    if (titleText() == cleaned)
        return;
    setTitleText(cleaned);
    emit titleChanged(cleaned);
}

QString ConfirmationDialog::message() const
{
    return messageText();
}

void ConfirmationDialog::setMessage(const QString& message)
{
    const QString cleaned = message.trimmed();
    if (messageText() == cleaned)
        return;
    setMessageText(cleaned);
    emit messageChanged(cleaned);
}

QString ConfirmationDialog::informativeText() const
{
    return m_informativeText;
}

void ConfirmationDialog::setInformativeText(const QString& text)
{
    const QString cleaned = text.trimmed();
    if (m_informativeText == cleaned)
        return;
    m_informativeText = cleaned;
    updateLabels();
    emit informativeTextChanged(m_informativeText);
}

QString ConfirmationDialog::details() const
{
    return m_details;
}

void ConfirmationDialog::setDetails(const QString& details)
{
    const QString cleaned = details.trimmed();
    if (m_details == cleaned)
        return;
    m_details = cleaned;
    updateLabels();
    emit detailsChanged(m_details);
}

bool ConfirmationDialog::isDestructive() const
{
    return m_destructive;
}

void ConfirmationDialog::setDestructive(bool destructive)
{
    if (m_destructive == destructive)
        return;
    m_destructive = destructive;
    updateButtons();
    emit destructiveChanged(m_destructive);
}

void ConfirmationDialog::setConfirmButtonText(const QString& text)
{
    m_confirmText = text.trimmed();
    updateButtons();
}

void ConfirmationDialog::setCancelButtonText(const QString& text)
{
    m_cancelText = text.trimmed();
    updateButtons();
}

void ConfirmationDialog::updateLabels()
{
    if (m_informativeLabel) {
        m_informativeLabel->setVisible(!m_informativeText.isEmpty());
        m_informativeLabel->setText(m_informativeText);
    }

    if (m_detailsLabel) {
        m_detailsLabel->setVisible(!m_details.isEmpty());
        m_detailsLabel->setText(m_details);
    }
}

void ConfirmationDialog::updateButtons()
{
    if (m_confirmButton) {
        if (!m_confirmText.isEmpty())
            m_confirmButton->setText(m_confirmText);
        m_confirmButton->setDefault(true);
        m_confirmButton->setAutoDefault(true);
    }

    if (m_cancelButton) {
        if (!m_cancelText.isEmpty())
            m_cancelButton->setText(m_cancelText);
    }

    if (m_confirmButton)
        m_confirmButton->setProperty("destructive", m_destructive);
    if (m_confirmButton && m_confirmButton->style()) {
        m_confirmButton->style()->unpolish(m_confirmButton);
        m_confirmButton->style()->polish(m_confirmButton);
        m_confirmButton->update();
    }
}

} // namespace Utils
