// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QMetaType>
#include <QtGui/QColor>

namespace Canvas::Api {

struct CANVAS_EXPORT CanvasBlockStyle final {
    QColor fillColor;
    QColor outlineColor;
    QColor labelColor;
    double cornerRadius = -1.0;

    bool hasColors() const {
        return fillColor.isValid() || outlineColor.isValid() || labelColor.isValid();
    }
};

} // namespace Canvas::Api

Q_DECLARE_METATYPE(Canvas::Api::CanvasBlockStyle)
