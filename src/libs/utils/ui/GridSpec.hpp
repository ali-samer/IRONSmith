// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QMarginsF>
#include <QtCore/QSizeF>
#include <QtCore/Qt>

namespace Utils {

Q_NAMESPACE

enum class GridOrigin : unsigned char {
    BottomLeft,
    TopLeft
};
Q_ENUM_NS(GridOrigin)

struct UTILS_EXPORT GridRect final {
    int column = 0;
    int row = 0;
    int columnSpan = 1;
    int rowSpan = 1;

    bool isValid() const { return columnSpan > 0 && rowSpan > 0; }
};

struct UTILS_EXPORT GridSpec final {
    Q_GADGET

    Q_PROPERTY(int columns MEMBER columns)
    Q_PROPERTY(int rows MEMBER rows)
    Q_PROPERTY(Utils::GridOrigin origin MEMBER origin)
    Q_PROPERTY(QSizeF cellSize MEMBER cellSize)
    Q_PROPERTY(bool autoCellSize MEMBER autoCellSize)
    Q_PROPERTY(QSizeF cellSpacing MEMBER cellSpacing)
    Q_PROPERTY(QMarginsF outerMargin MEMBER outerMargin)

public:
    int columns = 0;
    int rows = 0;
    GridOrigin origin = GridOrigin::BottomLeft;
    QSizeF cellSize{};
    bool autoCellSize = true;
    QSizeF cellSpacing{};
    QMarginsF outerMargin{};

    bool isValid() const { return columns > 0 && rows > 0; }
    bool hasExplicitCellSize() const {
        return !autoCellSize && cellSize.width() > 0.0 && cellSize.height() > 0.0;
    }
};

} // namespace Utils

Q_DECLARE_METATYPE(Utils::GridOrigin)
Q_DECLARE_METATYPE(Utils::GridRect)
Q_DECLARE_METATYPE(Utils::GridSpec)
