#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QString>
#include <QtWidgets/QDialog>

class QLabel;
class QDialogButtonBox;
class QPushButton;

namespace Utils {

struct UTILS_EXPORT ConfirmationDialogConfig final {
    QString title;
    QString message;
    QString informativeText;
    QString details;
    QString confirmText;
    QString cancelText;
    bool destructive = false;
};

class UTILS_EXPORT ConfirmationDialog final : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText NOTIFY informativeTextChanged)
    Q_PROPERTY(QString details READ details WRITE setDetails NOTIFY detailsChanged)
    Q_PROPERTY(bool destructive READ isDestructive WRITE setDestructive NOTIFY destructiveChanged)

public:
    explicit ConfirmationDialog(QWidget* parent = nullptr);

    static bool confirm(QWidget* parent, const ConfirmationDialogConfig& config);
    static bool confirmDelete(QWidget* parent, const QString& targetName, bool isFolder);

    QString title() const;
    void setTitle(const QString& title);

    QString message() const;
    void setMessage(const QString& message);

    QString informativeText() const;
    void setInformativeText(const QString& text);

    QString details() const;
    void setDetails(const QString& details);

    bool isDestructive() const;
    void setDestructive(bool destructive);

    void setConfirmButtonText(const QString& text);
    void setCancelButtonText(const QString& text);

signals:
    void titleChanged(const QString& title);
    void messageChanged(const QString& message);
    void informativeTextChanged(const QString& text);
    void detailsChanged(const QString& details);
    void destructiveChanged(bool destructive);

private:
    void updateLabels();
    void updateButtons();

    QString m_title;
    QString m_message;
    QString m_informativeText;
    QString m_details;
    QString m_confirmText;
    QString m_cancelText;
    bool m_destructive = false;

    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QLabel* m_informativeLabel = nullptr;
    QLabel* m_detailsLabel = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
    QPushButton* m_confirmButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace Utils
