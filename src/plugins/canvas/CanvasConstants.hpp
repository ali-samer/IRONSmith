#pragma once

namespace Canvas::Constants {

inline constexpr char CANVAS_BACKGROUND_COLOR[] = "#121316";

inline constexpr double kGridStep = 8.0;

inline constexpr double kMinZoom = 0.10;
inline constexpr double kMaxZoom = 8.00;
inline constexpr double kZoomStep = 1.10;

// Block styling (world-space colors; widths are adjusted in CanvasStyle).
inline constexpr char kBlockOutlineColor[] = "#2EC27E";
inline constexpr char kBlockFillColor[] = "#0E1B18";
inline constexpr char kBlockTextColor[] = "#B8C2CC";
inline constexpr char kBlockSelectionColor[] = "#5DA9FF";
inline constexpr char kWireColor[] = "#8D99A8";
inline constexpr char kDynamicPortColor[] = "#D2B36A";
inline constexpr double kBlockCornerRadius = 6.0;
inline constexpr double kBlockLabelPointSize = 10.0;
inline constexpr double kPortHitRadiusPx = 8.0;
inline constexpr double kEdgeHoverRadiusPx = 14.0;
inline constexpr double kEndpointHitRadiusPx = 10.0;
inline constexpr double kEndpointDragThresholdPx = 4.0;
inline constexpr double kMarqueeDragThresholdPx = 4.0;
inline constexpr double kPortActivationBandPx = 20.0;

inline constexpr double kPortStubLength = 10.0;
inline constexpr double kPortStubLengthHover = 14.0;
inline constexpr double kPortBoxHalf = 3.0;
inline constexpr double kPortBoxHalfHover = 4.0;
inline constexpr double kPortHitStubLengthPx = kPortStubLengthHover;
inline constexpr double kPortHitBoxHalfPx = kPortBoxHalfHover;

inline constexpr double kBlockKeepoutMargin = kGridStep;

inline constexpr double kLinkHubSize = kGridStep * 2.0;

inline constexpr char kLinkHubSplitFill[] = "#1B1422";
inline constexpr char kLinkHubSplitOutline[] = "#C089FF";
inline constexpr char kLinkHubSplitText[] = "#F1E6FF";
inline constexpr char kLinkHubJoinFill[] = "#201A12";
inline constexpr char kLinkHubJoinOutline[] = "#F2B65E";
inline constexpr char kLinkHubJoinText[] = "#FFE6B8";
inline constexpr char kLinkHubBroadcastFill[] = "#141D24";
inline constexpr char kLinkHubBroadcastOutline[] = "#58B5FF";
inline constexpr char kLinkHubBroadcastText[] = "#D5ECFF";

inline constexpr char kLinkWireProducerColor[] = "#E55353";
inline constexpr char kLinkWireConsumerColor[] = "#5CCB7A";

} // namespace Canvas::Constants
