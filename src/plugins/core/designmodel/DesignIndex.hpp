#pragma once

#include "designmodel/DesignModelGlobal.hpp"
#include "designmodel/DesignId.hpp"
#include "designmodel/DesignEntities.hpp"
#include "designmodel/Tile.hpp"

#include <QtCore/QHash>
#include <QtCore/QVector>

namespace DesignModel {

class DESIGNMODEL_EXPORT DesignIndex final {
public:
	DesignIndex() = default;

	DesignIndex(const QVector<BlockId>& blockOrder,
				const QVector<LinkId>& linkOrder,
				const QHash<BlockId, Block>& blocks,
				const QHash<PortId, Port>& ports,
				const QHash<LinkId, Link>& links);

	const QVector<PortId>& portsForBlock(BlockId blockId) const noexcept;
	const QVector<LinkId>& linksForPort(PortId portId) const noexcept;
	BlockId blockAtTile(TileCoord coord) const noexcept;
	const QVector<TileCoord>& tilesForBlock(BlockId blockId) const noexcept;
	const QVector<TileCoord>& collidingTiles() const noexcept;

	bool isEmpty() const noexcept;

private:
	static QVector<TileCoord> computeOccupiedTiles(const Placement& placement);

	QHash<BlockId, QVector<PortId>> m_portsByBlock;
	QHash<PortId, QVector<LinkId>>  m_linksByPort;

	QHash<TileCoord, BlockId>       m_blockByTile;
	QHash<BlockId, QVector<TileCoord>> m_tilesByBlock;

	QVector<TileCoord> m_collidingTiles;

	static const QVector<PortId>&  emptyPorts() noexcept;
	static const QVector<LinkId>&  emptyLinks() noexcept;
	static const QVector<TileCoord>& emptyTiles() noexcept;
};

} // namespace DesignModel
