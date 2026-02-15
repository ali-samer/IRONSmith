// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/state/AieLayoutSettings.hpp"

#include "aieplugin/AieCanvasCoordinator.hpp"
#include "aieplugin/state/AiePanelState.hpp"

#include <utils/EnvironmentQtPolicy.hpp>

#include <QtCore/QJsonValue>

#include <cmath>

namespace Aie::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kStateName = u"aie/panelState"_s;
const QString kTileSpacingKey = u"tileSpacing"_s;
const QString kOuterMarginKey = u"outerMargin"_s;
const QString kHorizontalSpacingKey = u"horizontalSpacing"_s;
const QString kVerticalSpacingKey = u"verticalSpacing"_s;
const QString kOutwardSpreadKey = u"outwardSpread"_s;
const QString kAutoCellSizeKey = u"autoCellSize"_s;
const QString kCellSizeKey = u"cellSize"_s;
const QString kKeepoutKey = u"keepoutMargin"_s;

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) <= 1e-6;
}

} // namespace

LayoutSettings layoutDefaults()
{
    return LayoutSettings{};
}

LayoutSettings layoutFromCoordinator(const AieCanvasCoordinator& coordinator)
{
    LayoutSettings settings;
    settings.horizontalSpacing = coordinator.horizontalSpacing();
    settings.verticalSpacing = coordinator.verticalSpacing();
    settings.outwardSpread = coordinator.outwardSpread();
    settings.autoCellSize = coordinator.autoCellSize();
    settings.cellSize = coordinator.cellSize();
    settings.keepoutMargin = coordinator.keepoutMargin();
    return settings;
}

LayoutSettings layoutFromJson(const QJsonObject& obj, const LayoutSettings& fallback)
{
    LayoutSettings settings = fallback;

    if (obj.contains(kHorizontalSpacingKey))
        settings.horizontalSpacing = obj.value(kHorizontalSpacingKey).toDouble(settings.horizontalSpacing);
    if (obj.contains(kVerticalSpacingKey))
        settings.verticalSpacing = obj.value(kVerticalSpacingKey).toDouble(settings.verticalSpacing);
    if (obj.contains(kOutwardSpreadKey))
        settings.outwardSpread = obj.value(kOutwardSpreadKey).toDouble(settings.outwardSpread);

    if (!obj.contains(kHorizontalSpacingKey) && !obj.contains(kVerticalSpacingKey) && obj.contains(kTileSpacingKey)) {
        const double spacing = obj.value(kTileSpacingKey).toDouble(settings.horizontalSpacing);
        settings.horizontalSpacing = spacing;
        settings.verticalSpacing = spacing;
    }
    if (!obj.contains(kOutwardSpreadKey) && obj.contains(kOuterMarginKey))
        settings.outwardSpread = obj.value(kOuterMarginKey).toDouble(settings.outwardSpread);

    if (obj.contains(kAutoCellSizeKey))
        settings.autoCellSize = obj.value(kAutoCellSizeKey).toBool(settings.autoCellSize);
    if (obj.contains(kCellSizeKey))
        settings.cellSize = obj.value(kCellSizeKey).toDouble(settings.cellSize);
    if (obj.contains(kKeepoutKey))
        settings.keepoutMargin = obj.value(kKeepoutKey).toDouble(settings.keepoutMargin);

    return settings;
}

QJsonObject layoutToJson(const LayoutSettings& settings)
{
    QJsonObject obj;
    obj.insert(kHorizontalSpacingKey, settings.horizontalSpacing);
    obj.insert(kVerticalSpacingKey, settings.verticalSpacing);
    obj.insert(kOutwardSpreadKey, settings.outwardSpread);
    obj.insert(kAutoCellSizeKey, settings.autoCellSize);
    obj.insert(kCellSizeKey, settings.cellSize);
    obj.insert(kKeepoutKey, settings.keepoutMargin);
    return obj;
}

void applyLayout(AieCanvasCoordinator& coordinator, const LayoutSettings& settings)
{
    coordinator.setHorizontalSpacing(settings.horizontalSpacing);
    coordinator.setVerticalSpacing(settings.verticalSpacing);
    coordinator.setOutwardSpread(settings.outwardSpread);
    coordinator.setAutoCellSize(settings.autoCellSize);
    coordinator.setCellSize(settings.cellSize);
    coordinator.setKeepoutMargin(settings.keepoutMargin);
}

bool layoutEquals(const LayoutSettings& a, const LayoutSettings& b)
{
    return nearlyEqual(a.horizontalSpacing, b.horizontalSpacing)
        && nearlyEqual(a.verticalSpacing, b.verticalSpacing)
        && nearlyEqual(a.outwardSpread, b.outwardSpread)
        && nearlyEqual(a.cellSize, b.cellSize)
        && nearlyEqual(a.keepoutMargin, b.keepoutMargin)
        && (a.autoCellSize == b.autoCellSize);
}

LayoutSettings loadDefaultLayout()
{
    Utils::Environment env = AiePanelState::makeEnvironment();
    const auto loaded = env.loadState(Utils::EnvironmentScope::Global, kStateName);
    const LayoutSettings fallback = layoutDefaults();
    if (loaded.status != Utils::DocumentLoadResult::Status::Ok)
        return fallback;
    return layoutFromJson(loaded.object, fallback);
}

} // namespace Aie::Internal
