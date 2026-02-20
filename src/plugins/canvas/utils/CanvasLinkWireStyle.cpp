// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "canvas/utils/CanvasLinkWireStyle.hpp"

#include "canvas/CanvasConstants.hpp"

namespace Canvas::Support {

LinkWireStyle linkWireStyle(LinkWireRole role)
{
    switch (role) {
        case LinkWireRole::Producer:
            return {QColor(Constants::kLinkWireProducerColor)};
        case LinkWireRole::Consumer:
            return {QColor(Constants::kLinkWireConsumerColor)};
    }
    return {QColor(Constants::kWireColor)};
}

} // namespace Canvas::Support
