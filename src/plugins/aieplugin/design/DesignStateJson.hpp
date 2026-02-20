// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "aieplugin/design/DesignState.hpp"

#include <utils/Result.hpp>

#include <QtCore/QJsonObject>

namespace Aie::Internal {

Utils::Result parseDesignState(const QJsonObject& json, DesignState& out);
QJsonObject serializeDesignState(const DesignState& state);

} // namespace Aie::Internal
