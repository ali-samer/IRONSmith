// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/ui/BaseDialog.hpp"

class QFormLayout;

namespace Utils {

class UTILS_EXPORT FormDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit FormDialog(QWidget* parent = nullptr);

    QFormLayout* formLayout() const;

private:
    QFormLayout* m_formLayout = nullptr;
};

} // namespace Utils
