#pragma once

#include "utils/UtilsGlobal.hpp"
#include "utils/ui/BaseDialog.hpp"

#include <QtCore/QString>
#include <qnamespace.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

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

class UTILS_EXPORT ConfirmationDialog final : public BaseDialog
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

    QString m_informativeText;
    QString m_details;
    QString m_confirmText;
    QString m_cancelText;
    bool m_destructive = false;

    QLabel* m_informativeLabel = nullptr;
    QLabel* m_detailsLabel = nullptr;
    QPushButton* m_confirmButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace Utils
