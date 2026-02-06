#include "utils/ui/ConfirmationDialog.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QStyle>

namespace Utils {

ConfirmationDialog::ConfirmationDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("ConfirmationDialog"));
    setAttribute(Qt::WA_StyledBackground, true);
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("ConfirmationDialogTitle"));
    QFont titleFont = m_titleLabel->font();
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setVisible(false);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("ConfirmationDialogMessage"));
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);

    m_informativeLabel = new QLabel(this);
    m_informativeLabel->setObjectName(QStringLiteral("ConfirmationDialogInformative"));
    m_informativeLabel->setWordWrap(true);
    m_informativeLabel->setVisible(false);

    m_detailsLabel = new QLabel(this);
    m_detailsLabel->setObjectName(QStringLiteral("ConfirmationDialogDetails"));
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setVisible(false);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_messageLabel);
    layout->addWidget(m_informativeLabel);
    layout->addWidget(m_detailsLabel);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->setObjectName(QStringLiteral("ConfirmationDialogButtons"));
    m_confirmButton = m_buttons->button(QDialogButtonBox::Ok);
    m_cancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    if (m_confirmButton)
        m_confirmButton->setObjectName(QStringLiteral("ConfirmationDialogConfirmButton"));
    if (m_cancelButton)
        m_cancelButton->setObjectName(QStringLiteral("ConfirmationDialogCancelButton"));

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttons);

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
    return m_title;
}

void ConfirmationDialog::setTitle(const QString& title)
{
    const QString cleaned = title.trimmed();
    if (m_title == cleaned)
        return;
    m_title = cleaned;
    setWindowTitle(m_title);
    updateLabels();
    emit titleChanged(m_title);
}

QString ConfirmationDialog::message() const
{
    return m_message;
}

void ConfirmationDialog::setMessage(const QString& message)
{
    const QString cleaned = message.trimmed();
    if (m_message == cleaned)
        return;
    m_message = cleaned;
    updateLabels();
    emit messageChanged(m_message);
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
    if (m_titleLabel) {
        m_titleLabel->setVisible(!m_title.isEmpty());
        m_titleLabel->setText(m_title);
    }

    if (m_messageLabel) {
        m_messageLabel->setVisible(!m_message.isEmpty());
        m_messageLabel->setText(m_message);
    }

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
