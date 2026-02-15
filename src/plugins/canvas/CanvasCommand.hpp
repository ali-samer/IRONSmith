// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QString>

namespace Canvas {

class CanvasDocument;

class CANVAS_EXPORT CanvasCommand
{
public:
    virtual ~CanvasCommand() = default;

    virtual QString name() const = 0;

    virtual bool apply(CanvasDocument& doc) = 0;

    virtual bool revert(CanvasDocument& doc) = 0;
};

} // namespace Canvas
