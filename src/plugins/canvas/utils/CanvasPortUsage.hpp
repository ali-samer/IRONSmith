// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

namespace Canvas {

class CanvasDocument;

namespace Support {

int countPortAttachments(const CanvasDocument& doc,
                         ObjectId itemId,
                         PortId portId,
                         ObjectId excludeWireId = {});

bool isPortAvailable(const CanvasDocument& doc,
                     ObjectId itemId,
                     PortId portId,
                     ObjectId excludeWireId = {});

} // namespace Support
} // namespace Canvas
