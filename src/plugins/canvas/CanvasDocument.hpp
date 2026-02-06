#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>

#include "canvas/CanvasFabric.hpp"
#include "canvas/CanvasCommandManager.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasItem.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace Canvas {

class CanvasItem;
class CanvasBlock;
struct PortRef;
struct CanvasPort;

class CANVAS_EXPORT CanvasDocument final : public QObject
{
	Q_OBJECT

public:
	explicit CanvasDocument(QObject* parent = nullptr);

	const CanvasFabric& fabric() const { return m_fabric; }
	CanvasFabric& fabric() { return m_fabric; }

	const std::vector<std::unique_ptr<CanvasItem>>& items() const { return m_items; }
	CanvasBlock* createBlock(const QRectF& boundsScene, bool movable);

	CanvasCommandManager& commands() { return m_commands; }
	const CanvasCommandManager& commands() const { return m_commands; }

	CanvasItem* hitTest(const QPointF& scenePos) const;
	std::optional<PortRef> hitTestPort(const QPointF& scenePos, double radiusScene) const;
	bool getPort(ObjectId itemId, PortId portId, CanvasPort& out) const;

	struct RemovedItem final {
		std::unique_ptr<CanvasItem> item;
		size_t index = 0;
	};

	std::optional<RemovedItem> removeItem(ObjectId itemId);
	bool insertItem(size_t index, std::unique_ptr<CanvasItem> item);

	ObjectId allocateId() { return nextId(); }

	bool setItemTopLeft(ObjectId itemId, const QPointF& newTopLeftScene);
	bool previewSetItemTopLeft(ObjectId itemId, const QPointF& newTopLeftScene);

	bool isFabricPointBlocked(const FabricCoord& coord) const;
	static bool isFabricPointBlockedThunk(const FabricCoord& coord, void* user);

	bool computePortTerminal(ObjectId itemId, PortId portId,
	                         QPointF& outAnchorScene,
	                         QPointF& outBorderScene,
	                         QPointF& outFabricScene) const;
	static bool computePortTerminalThunk(void* user, ObjectId itemId, PortId portId,
	                                    QPointF& outAnchorScene,
	                                    QPointF& outBorderScene,
	                                    QPointF& outFabricScene);

	QString statusText() const;
	void setStatusText(QString text);

	ObjectId nextId();
	CanvasItem* findItem(ObjectId id) const;

signals:
	void changed();

private:
	friend class CanvasCommandManager;
	bool setItemTopLeftImpl(CanvasItem* item, const QPointF& newTopLeftScene, bool emitChanged);
	void arrangeAutoPorts(CanvasBlock& block) const;

	QString m_statusText;
	CanvasFabric m_fabric;
	std::vector<std::unique_ptr<CanvasItem>> m_items;
	CanvasCommandManager m_commands;
};

} // namespace Canvas
