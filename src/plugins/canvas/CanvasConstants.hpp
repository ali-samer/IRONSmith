#pragma once

namespace Canvas::Constants {

inline constexpr char CANVAS_BACKGROUND_COLOR[] = "#121316";

inline constexpr double kGridStep = 10.0;

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

inline constexpr double kBlockKeepoutMargin = kGridStep;

inline constexpr double kLinkHubSize = kGridStep * 2.0;

} // namespace Canvas::Constants
