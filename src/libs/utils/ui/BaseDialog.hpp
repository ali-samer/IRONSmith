// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QString>
#include <QtWidgets/QDialog>

class QLabel;
class QDialogButtonBox;
class QVBoxLayout;
class QWidget;

namespace Utils {

class UTILS_EXPORT BaseDialog : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString titleText READ titleText WRITE setTitleText NOTIFY titleTextChanged)
    Q_PROPERTY(QString messageText READ messageText WRITE setMessageText NOTIFY messageTextChanged)

public:
    explicit BaseDialog(QWidget* parent = nullptr);

    QString titleText() const;
    void setTitleText(const QString& text);

    QString messageText() const;
    void setMessageText(const QString& text);

    QLabel* titleLabel() const;
    QLabel* messageLabel() const;

    QWidget* contentWidget() const;
    QVBoxLayout* contentLayout() const;
    QDialogButtonBox* buttonBox() const;

signals:
    void titleTextChanged(const QString& text);
    void messageTextChanged(const QString& text);

protected:
    void updateHeader();

private:
    QString m_titleText;
    QString m_messageText;

    QWidget* m_header = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;

    QWidget* m_content = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;
};

} // namespace Utils
