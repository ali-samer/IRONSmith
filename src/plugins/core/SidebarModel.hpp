// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <functional>

#include "core/api/SidebarToolSpec.hpp"
#include "core/CoreGlobal.hpp"

class QWidget;

namespace Core {

class CORE_EXPORT SidebarModel final : public QObject
{
    Q_OBJECT

public:
    using PanelFactory = std::function<QWidget*(QWidget* parent)>;

    struct PlacementKey
    {
        SidebarSide side = SidebarSide::Left;
        SidebarFamily family = SidebarFamily::Vertical;
        SidebarRegion region = SidebarRegion::Exclusive;
        SidebarRail rail = SidebarRail::Top;

        friend bool operator==(const PlacementKey& a, const PlacementKey& b) noexcept
        {
            return a.side == b.side && a.family == b.family && a.region == b.region && a.rail == b.rail;
        }
    };

    explicit SidebarModel(QObject* parent = nullptr);
    ~SidebarModel() override = default;

    // Registration
    bool registerTool(const SidebarToolSpec& spec,
                      PanelFactory factory,
                      QString* errorOut = nullptr);

    bool unregisterTool(const QString& id, QString* errorOut = nullptr);

    bool hasTool(const QString& id) const noexcept;
    const SidebarToolSpec* toolSpec(const QString& id) const noexcept;

    QVector<QString> toolIdsForRail(SidebarSide side, SidebarFamily family, SidebarRail rail) const;

    // Panel state
    bool isOpen(const QString& id) const noexcept;
    bool isActiveExclusive(const QString& id) const noexcept;

    QString activeToolId(SidebarSide side, SidebarFamily family, SidebarRegion region) const;

    bool requestShowTool(const QString& id, QString* errorOut = nullptr);
    bool requestHideTool(const QString& id, QString* errorOut = nullptr);
    bool requestToggleTool(const QString& id, QString* errorOut = nullptr);

    PanelFactory panelFactory(const QString& id) const;

signals:
    // Structure
    void toolRegistered(const QString& id);
    void toolUnregistered(const QString& id);
    void railToolsChanged(SidebarSide side, SidebarFamily family, SidebarRail rail);

    // State
    void toolOpenStateChanged(const QString& id, bool open);
    void exclusiveActiveChanged(SidebarSide side, SidebarFamily family, SidebarRegion region, const QString& activeId);

private:
    struct ToolEntry
    {
        SidebarToolSpec spec;
        PanelFactory factory;

        bool open = false;
    };

    static bool isValidId(const QString& id);

    static PlacementKey keyFor(const SidebarToolSpec& spec);

    bool setExclusiveActive(const QString& id, bool allowClear, QString* errorOut);
    bool setAdditiveOpen(const QString& id, bool open, QString* errorOut);

    void invalidateRailCacheFor(const SidebarToolSpec& spec) const;
    QVector<QString> computeRailToolIds(SidebarSide side, SidebarFamily family, SidebarRail rail) const;

    QHash<QString, ToolEntry> m_toolsById;

    struct ExclusiveKey
    {
        SidebarSide side = SidebarSide::Left;
        SidebarFamily family = SidebarFamily::Vertical;
        SidebarRegion region = SidebarRegion::Exclusive;

        friend bool operator==(const ExclusiveKey& a, const ExclusiveKey& b) noexcept
        {
            return a.side == b.side && a.family == b.family && a.region == b.region;
        }
    };

    QHash<ExclusiveKey, QString> m_activeExclusiveByKey;

    struct AdditiveKey
    {
        SidebarSide side = SidebarSide::Left;
        SidebarFamily family = SidebarFamily::Vertical;

        friend bool operator==(const AdditiveKey& a, const AdditiveKey& b) noexcept
        {
            return a.side == b.side && a.family == b.family;
        }
    };

    QHash<AdditiveKey, QString> m_activeAdditiveByKey;

    struct RailKey
    {
        SidebarSide side = SidebarSide::Left;
        SidebarFamily family = SidebarFamily::Vertical;
        SidebarRail rail = SidebarRail::Top;

        friend bool operator==(const RailKey& a, const RailKey& b) noexcept
        {
            return a.side == b.side && a.family == b.family && a.rail == b.rail;
        }
    };

    struct RailCache
    {
        bool dirty = true;
        QVector<QString> ids;
    };

    friend size_t qHash(const ExclusiveKey& k, size_t seed) noexcept;
    friend size_t qHash(const AdditiveKey& k, size_t seed) noexcept;
    friend size_t qHash(const RailKey& k, size_t seed) noexcept;
    mutable QHash<RailKey, RailCache> m_railCache;
};

} // namespace Core
