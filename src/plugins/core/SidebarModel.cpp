#include "SidebarModel.hpp"

#include <QtCore/QDebug>
namespace Core {

size_t qHash(const SidebarModel::ExclusiveKey& k, size_t seed) noexcept
{
    return ::qHash(int(k.side) * 31 + int(k.family) * 7 + int(k.region), seed);
}

size_t qHash(const SidebarModel::AdditiveKey& k, size_t seed) noexcept
{
    return ::qHash(int(k.side) * 31 + int(k.family) * 7 + 1, seed);
}

size_t qHash(const SidebarModel::RailKey& k, size_t seed) noexcept
{
    return ::qHash(int(k.side) * 31 + int(k.family) * 7 + int(k.rail), seed);
}

} // namespace Core


namespace Core {

SidebarModel::SidebarModel(QObject* parent)
    : QObject(parent)
{
}

bool SidebarModel::isValidId(const QString& id)
{
    if (id.trimmed().isEmpty())
        return false;

    for (const QChar c : id) {
        if (!(c.isLetterOrNumber() || c == '_' || c == '-' || c == '.'))
            return false;
    }
    return true;
}

SidebarModel::PlacementKey SidebarModel::keyFor(const SidebarToolSpec& spec)
{
    return PlacementKey{spec.side, spec.family, spec.region, spec.rail};
}

bool SidebarModel::registerTool(const SidebarToolSpec& spec,
                                PanelFactory factory,
                                QString* errorOut)
{
    auto fail = [&](QString msg) {
        if (errorOut) *errorOut = std::move(msg);
        return false;
    };

    if (!isValidId(spec.id))
        return fail(QString("Sidebar tool id is invalid: '%1'. Use [A-Za-z0-9_.-] and non-empty.").arg(spec.id));

    if (m_toolsById.contains(spec.id))
        return fail(QString("Sidebar tool id already registered: '%1'.").arg(spec.id));

    if (!factory)
        return fail(QString("Sidebar tool '%1' registration failed: panel factory is empty.").arg(spec.id));

    ToolEntry e;
    e.spec = spec;
    e.factory = std::move(factory);
    e.open = false;

    m_toolsById.insert(spec.id, std::move(e));

    invalidateRailCacheFor(spec);

    emit toolRegistered(spec.id);
    emit railToolsChanged(spec.side, spec.family, spec.rail);

    return true;
}

bool SidebarModel::unregisterTool(const QString& id, QString* errorOut)
{
    auto fail = [&](QString msg) {
        if (errorOut) *errorOut = std::move(msg);
        return false;
    };

    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end())
        return fail(QString("Sidebar tool id not registered: '%1'.").arg(id));

    const SidebarToolSpec spec = it->spec;

    const ExclusiveKey exKey{spec.side, spec.family, spec.region};
    const bool wasActiveExclusive = (spec.region == SidebarRegion::Exclusive)
                                    && (m_activeExclusiveByKey.value(exKey) == id);

    const AdditiveKey addKey{spec.side, spec.family};
    const bool wasActiveAdditive = (spec.region == SidebarRegion::Additive)
                                   && (m_activeAdditiveByKey.value(addKey) == id);

    if (wasActiveExclusive) {
        m_activeExclusiveByKey.remove(exKey);
        emit exclusiveActiveChanged(spec.side, spec.family, spec.region, QString());
        emit toolOpenStateChanged(id, false);
    }

    if (wasActiveAdditive) {
        m_activeAdditiveByKey.remove(addKey);
        emit toolOpenStateChanged(id, false);
    }

    if (!wasActiveExclusive && !wasActiveAdditive && isOpen(id))
        emit toolOpenStateChanged(id, false);

    m_toolsById.erase(it);

    invalidateRailCacheFor(spec);

    emit toolUnregistered(id);
    emit railToolsChanged(spec.side, spec.family, spec.rail);

    return true;
}

bool SidebarModel::hasTool(const QString& id) const noexcept
{
    return m_toolsById.contains(id);
}

const SidebarToolSpec* SidebarModel::toolSpec(const QString& id) const noexcept
{
    auto it = m_toolsById.constFind(id);
    if (it == m_toolsById.constEnd())
        return nullptr;
    return &it->spec;
}

SidebarModel::PanelFactory SidebarModel::panelFactory(const QString& id) const
{
    auto it = m_toolsById.constFind(id);
    if (it == m_toolsById.constEnd())
        return {};
    return it->factory;
}

void SidebarModel::invalidateRailCacheFor(const SidebarToolSpec& spec) const
{
    const RailKey key{spec.side, spec.family, spec.rail};
    auto& cache = m_railCache[key];
    cache.dirty = true;
}

QVector<QString> SidebarModel::computeRailToolIds(SidebarSide side, SidebarFamily family, SidebarRail rail) const
{
    struct Item { QString id; int order; };

    QVector<Item> items;
    items.reserve(m_toolsById.size());

    for (auto it = m_toolsById.constBegin(); it != m_toolsById.constEnd(); ++it) {
        const SidebarToolSpec& s = it->spec;
        if (s.side != side || s.family != family || s.rail != rail)
            continue;
        items.push_back(Item{s.id, s.order});
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.order != b.order) return a.order < b.order;
        return a.id < b.id;
    });

    QVector<QString> out;
    out.reserve(items.size());
    for (const auto& x : items)
        out.push_back(x.id);

    return out;
}

QVector<QString> SidebarModel::toolIdsForRail(SidebarSide side, SidebarFamily family, SidebarRail rail) const
{
    const RailKey key{side, family, rail};
    auto& cache = m_railCache[key];
    if (cache.dirty) {
        cache.ids = computeRailToolIds(side, family, rail);
        cache.dirty = false;
    }
    return cache.ids;
}

bool SidebarModel::isOpen(const QString& id) const noexcept
{
    auto it = m_toolsById.constFind(id);
    if (it == m_toolsById.constEnd())
        return false;

    const SidebarToolSpec& spec = it->spec;

    if (spec.region == SidebarRegion::Exclusive) {
        const ExclusiveKey exKey{spec.side, spec.family, spec.region};
        return m_activeExclusiveByKey.value(exKey) == id;
    }

    const AdditiveKey addKey{spec.side, spec.family};
    return m_activeAdditiveByKey.value(addKey) == id;
}

bool SidebarModel::isActiveExclusive(const QString& id) const noexcept
{
    auto it = m_toolsById.constFind(id);
    if (it == m_toolsById.constEnd())
        return false;

    const SidebarToolSpec& spec = it->spec;
    if (spec.region != SidebarRegion::Exclusive)
        return false;

    const ExclusiveKey exKey{spec.side, spec.family, spec.region};
    return m_activeExclusiveByKey.value(exKey) == id;
}

QString SidebarModel::activeToolId(SidebarSide side, SidebarFamily family, SidebarRegion region) const
{
    if (region == SidebarRegion::Exclusive) {
        const ExclusiveKey exKey{side, family, region};
        return m_activeExclusiveByKey.value(exKey);
    }

    const AdditiveKey addKey{side, family};
    return m_activeAdditiveByKey.value(addKey);
}

bool SidebarModel::requestShowTool(const QString& id, QString* errorOut)
{
    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end()) {
        if (errorOut) *errorOut = QString("Unknown sidebar tool id: '%1'.").arg(id);
        return false;
    }

    if (it->spec.region == SidebarRegion::Exclusive)
        return setExclusiveActive(id, /*allowClear=*/false, errorOut);

    return setAdditiveOpen(id, true, errorOut);
}

bool SidebarModel::requestHideTool(const QString& id, QString* errorOut)
{
    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end()) {
        if (errorOut) *errorOut = QString("Unknown sidebar tool id: '%1'.").arg(id);
        return false;
    }

    if (it->spec.region == SidebarRegion::Exclusive) {
        const ExclusiveKey exKey{it->spec.side, it->spec.family, it->spec.region};
        const QString current = m_activeExclusiveByKey.value(exKey);

        if (current != id)
            return true;

        return setExclusiveActive(id, /*allowClear=*/true, errorOut);
    }

    return setAdditiveOpen(id, false, errorOut);
}

bool SidebarModel::requestToggleTool(const QString& id, QString* errorOut)
{
    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end()) {
        if (errorOut) *errorOut = QString("Unknown sidebar tool id: '%1'.").arg(id);
        return false;
    }

    if (it->spec.region == SidebarRegion::Exclusive) {
        const ExclusiveKey exKey{it->spec.side, it->spec.family, it->spec.region};
        const QString current = m_activeExclusiveByKey.value(exKey);
        const bool alreadyActive = (current == id);

        if (alreadyActive)
            return setExclusiveActive(id, /*allowClear=*/true, errorOut);

        return setExclusiveActive(id, /*allowClear=*/false, errorOut);
    }

    const AdditiveKey addKey{it->spec.side, it->spec.family};
    const QString current = m_activeAdditiveByKey.value(addKey);
    const bool alreadyActive = (current == id);
    return setAdditiveOpen(id, !alreadyActive, errorOut);
}

bool SidebarModel::setExclusiveActive(const QString& id, bool allowClear, QString* errorOut)
{
    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end()) {
        if (errorOut) *errorOut = QString("Unknown sidebar tool id: '%1'.").arg(id);
        return false;
    }

    const SidebarToolSpec& spec = it->spec;
    if (spec.region != SidebarRegion::Exclusive) {
        if (errorOut) *errorOut = QString("Tool '%1' is not in an exclusive region.").arg(id);
        return false;
    }

    const ExclusiveKey exKey{spec.side, spec.family, spec.region};
    const QString current = m_activeExclusiveByKey.value(exKey);

    if (current == id) {
        if (!allowClear)
            return true; // idempotent

        m_activeExclusiveByKey.remove(exKey);
        emit exclusiveActiveChanged(spec.side, spec.family, spec.region, QString());
        emit toolOpenStateChanged(id, false);
        return true;
    }

    if (!current.isEmpty())
        emit toolOpenStateChanged(current, false);

    m_activeExclusiveByKey.insert(exKey, id);

    emit exclusiveActiveChanged(spec.side, spec.family, spec.region, id);
    emit toolOpenStateChanged(id, true);

    return true;
}

bool SidebarModel::setAdditiveOpen(const QString& id, bool open, QString* errorOut)
{
    auto it = m_toolsById.find(id);
    if (it == m_toolsById.end()) {
        if (errorOut) *errorOut = QString("Unknown sidebar tool id: '%1'.").arg(id);
        return false;
    }

    const SidebarToolSpec& spec = it->spec;
    if (spec.region != SidebarRegion::Additive) {
        if (errorOut) *errorOut = QString("Tool '%1' is not in an additive region.").arg(id);
        return false;
    }

    const AdditiveKey addKey{spec.side, spec.family};
    const QString current = m_activeAdditiveByKey.value(addKey);
    const bool alreadyActive = (current == id);

    if (open && alreadyActive)
        return true;

    if (!open && !alreadyActive)
        return true;

    if (open) {
        if (!current.isEmpty()) {
            if (auto jt = m_toolsById.find(current); jt != m_toolsById.end())
                jt->open = false;
            emit toolOpenStateChanged(current, false);
        }

        it->open = true;
        m_activeAdditiveByKey.insert(addKey, id);
        emit toolOpenStateChanged(id, true);
        return true;
    }

    it->open = false;
    m_activeAdditiveByKey.remove(addKey);
    emit toolOpenStateChanged(id, false);
    return true;
}

} // namespace Core
