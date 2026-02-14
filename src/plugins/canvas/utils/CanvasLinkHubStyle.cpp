#include "canvas/utils/CanvasLinkHubStyle.hpp"

#include "canvas/CanvasConstants.hpp"

namespace Canvas::Support {

LinkHubStyle linkHubStyle(LinkHubKind kind)
{
    switch (kind) {
        case LinkHubKind::Split:
            return {QStringLiteral("S"),
                    QColor(Constants::kLinkHubSplitFill),
                    QColor(Constants::kLinkHubSplitOutline),
                    QColor(Constants::kLinkHubSplitText)};
        case LinkHubKind::Join:
            return {QStringLiteral("J"),
                    QColor(Constants::kLinkHubJoinFill),
                    QColor(Constants::kLinkHubJoinOutline),
                    QColor(Constants::kLinkHubJoinText)};
        case LinkHubKind::Broadcast:
            return {QStringLiteral("B"),
                    QColor(Constants::kLinkHubBroadcastFill),
                    QColor(Constants::kLinkHubBroadcastOutline),
                    QColor(Constants::kLinkHubBroadcastText)};
    }
    return {QStringLiteral("?"),
            QColor(Constants::kBlockFillColor),
            QColor(Constants::kBlockOutlineColor),
            QColor(Constants::kBlockTextColor)};
}

} // namespace Canvas::Support
