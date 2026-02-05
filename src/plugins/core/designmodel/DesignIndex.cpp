#include "canvas/designmodel/DesignIndex.hpp"

#include <QtCore/QSet>

namespace DesignModel {

static void appendStable(QVector<TileCoord>& dst, const QVector<TileCoord>& src) {
    dst.reserve(dst.size() + src.size());
    for (const auto& t : src)
        dst.push_back(t);
}

const QVector<PortId>& DesignIndex::emptyPorts() noexcept {
    static const QVector<PortId> kEmpty{};
    return kEmpty;
}
const QVector<LinkId>& DesignIndex::emptyLinks() noexcept {
    static const QVector<LinkId> kEmpty{};
    return kEmpty;
}
const QVector<TileCoord>& DesignIndex::emptyTiles() noexcept {
    static const QVector<TileCoord> kEmpty{};
    return kEmpty;
}

QVector<TileCoord> DesignIndex::computeOccupiedTiles(const Placement& placement) {
    QVector<TileCoord> out;
    if (!placement.isValid())
        return out;

    const auto anchor = placement.anchor();
    out.reserve(placement.rowSpan() * placement.colSpan());

    for (int r = 0; r < placement.rowSpan(); ++r) {
        for (int c = 0; c < placement.colSpan(); ++c) {
            out.push_back(TileCoord(anchor.row() + r, anchor.col() + c));
        }
    }
    return out;
}

DesignIndex::DesignIndex(const QVector<BlockId>& blockOrder,
                         const QVector<LinkId>& linkOrder,
                         const QHash<BlockId, Block>& blocks,
                         const QHash<PortId, Port>& ports,
                         const QHash<LinkId, Link>& links)
{
    m_portsByBlock.reserve(blockOrder.size());
    for (const BlockId bid : blockOrder) {
        const auto bit = blocks.constFind(bid);
        if (bit == blocks.constEnd())
            continue;

        const Block& b = *bit;
        QVector<PortId> owned;
        owned.reserve(b.ports().size());
        for (const PortId pid : b.ports()) {
            if (ports.contains(pid))
                owned.push_back(pid);
        }
        if (!owned.isEmpty())
            m_portsByBlock.insert(bid, std::move(owned));
    }

    m_linksByPort.reserve(ports.size());
    for (const LinkId lid : linkOrder) {
        const auto lit = links.constFind(lid);
        if (lit == links.constEnd())
            continue;

        const Link& l = *lit;
        if (!l.isValid())
            continue;

        m_linksByPort[l.from()].push_back(lid);
        m_linksByPort[l.to()].push_back(lid);
    }

    QSet<TileCoord> collisions;
    for (const BlockId bid : blockOrder) {
        const auto bit = blocks.constFind(bid);
        if (bit == blocks.constEnd())
            continue;

        const Block& b = *bit;
        if (!b.isValid())
            continue;

        const QVector<TileCoord> occ = computeOccupiedTiles(b.placement());
        if (!occ.isEmpty())
            m_tilesByBlock.insert(bid, occ);

        for (const TileCoord t : occ) {
            if (m_blockByTile.contains(t)) {
                collisions.insert(t);
            } else {
                m_blockByTile.insert(t, bid);
            }
        }
    }

    m_collidingTiles = collisions.values().toVector();
    std::ranges::sort(m_collidingTiles,
                      [](const TileCoord& a, const TileCoord& b) { return (a < b); });
}

const QVector<PortId>& DesignIndex::portsForBlock(BlockId blockId) const noexcept {
    const auto it = m_portsByBlock.constFind(blockId);
    return it == m_portsByBlock.constEnd() ? emptyPorts() : *it;
}

const QVector<LinkId>& DesignIndex::linksForPort(PortId portId) const noexcept {
    const auto it = m_linksByPort.constFind(portId);
    return it == m_linksByPort.constEnd() ? emptyLinks() : *it;
}

BlockId DesignIndex::blockAtTile(TileCoord coord) const noexcept {
    const auto it = m_blockByTile.constFind(coord);
    return it == m_blockByTile.constEnd() ? BlockId::null() : *it;
}

const QVector<TileCoord>& DesignIndex::tilesForBlock(BlockId blockId) const noexcept {
    const auto it = m_tilesByBlock.constFind(blockId);
    return it == m_tilesByBlock.constEnd() ? emptyTiles() : *it;
}

const QVector<TileCoord>& DesignIndex::collidingTiles() const noexcept {
    return m_collidingTiles;
}

bool DesignIndex::isEmpty() const noexcept {
    return m_portsByBlock.isEmpty() && m_linksByPort.isEmpty() && m_blockByTile.isEmpty();
}

} // namespace DesignModel