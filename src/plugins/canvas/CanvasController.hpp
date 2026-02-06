#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasWire.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPoint>
#include <vector>
#include <optional>

namespace Canvas {

class CanvasDocument;
class CanvasView;
class CanvasBlock;
struct PortRef;

struct EdgeCandidate final {
	ObjectId itemId{};
	PortSide side = PortSide::Left;
	double t = 0.5;
	QPointF anchorScene;
};

class CANVAS_EXPORT CanvasController final : public QObject
{
	Q_OBJECT

public:
	enum class Mode { Normal, Panning, Linking };
	Q_ENUM(Mode)

	enum class LinkingMode { Normal, Split, Join, Broadcast };
	Q_ENUM(LinkingMode)

	CanvasController(CanvasDocument* doc, CanvasView* view, QObject* parent = nullptr);

	Mode mode() const noexcept { return m_mode; }
	LinkingMode linkingMode() const noexcept { return m_linkingMode; }
	bool isLinkingInProgress() const noexcept { return m_wiring; }
	bool isEndpointDragActive() const noexcept { return m_dragEndpoint; }
	ObjectId linkStartItem() const noexcept { return m_wireStartItem; }
	PortId linkStartPort() const noexcept { return m_wireStartPort; }
	QPointF linkPreviewScene() const noexcept { return m_wirePreviewScene; }

public slots:
	void onCanvasMousePressed(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void onCanvasMouseMoved(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void onCanvasMouseReleased(const QPointF& scenePos, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
	void onCanvasWheel(const QPointF& scenePos, const QPoint& angleDelta, const QPoint& pixelDelta, Qt::KeyboardModifiers mods);
	void onCanvasKeyPressed(int key, Qt::KeyboardModifiers mods);

signals:
	void modeChanged(CanvasController::Mode mode);
	void linkingModeChanged(CanvasController::LinkingMode mode);

private:
	void setMode(Mode mode);
	void setLinkingMode(LinkingMode mode);
	void resetLinkingSession();
	bool handleLinkingHubPress(const QPointF& scenePos, const PortRef& hitPort);
	bool beginLinkingFromPort(const PortRef& hitPort, const QPointF& scenePos);
	bool resolvePortTerminal(const PortRef& port,
	                         QPointF& outAnchor,
	                         QPointF& outBorder,
	                         QPointF& outFabric) const;
	std::unique_ptr<CanvasWire> buildWire(const PortRef& a, const PortRef& b) const;
	CanvasBlock* findLinkHub() const;
	bool connectToExistingHub(const QPointF& scenePos, const PortRef& hitPort);
	bool createHubAndWires(const QPointF& scenePos, const PortRef& hitPort);

	QPointF sceneToView(const QPointF& scenePos) const;
	void beginPanning(const QPointF& viewPos);
	void updatePanning(const QPointF& viewPos);
	void endPanning();

	bool handleLinkingPress(const QPointF& scenePos);
	void updateLinkingHoverAndPreview(const QPointF& scenePos);

	void selectItem(ObjectId id);
	void selectPort(const PortRef& port);
	void clearSelectedPort();
	void clearTransientDragState();
	void beginWireSegmentDrag(CanvasWire* wire, const QPointF& scenePos);
	void updateWireSegmentDrag(const QPointF& scenePos);
	void endWireSegmentDrag(const QPointF& scenePos);
	bool beginEndpointDrag(CanvasWire* wire, const QPointF& scenePos);
	void updateEndpointDrag(const QPointF& scenePos);
	void endEndpointDrag(const QPointF& scenePos);

	void beginBlockDrag(CanvasBlock* blk, const QPointF& scenePos);
	void updateBlockDrag(const QPointF& scenePos);
	void endBlockDrag();

	CanvasDocument* m_doc = nullptr;
	CanvasView* m_view = nullptr;

	bool    m_panning = false;
	QPointF m_lastViewPos;
	QPointF m_panStart;
	Mode m_mode = Mode::Normal;
	Mode m_modeBeforePan = Mode::Normal;
	LinkingMode m_linkingMode = LinkingMode::Normal;
	ObjectId m_linkHubId{};

	bool m_wiring = false;
	ObjectId m_wireStartItem{};
	PortId m_wireStartPort{};
	QPointF m_wirePreviewScene{};

	bool m_dragWire = false;
	ObjectId m_dragWireId{};
	int m_dragWireSeg = -1;
	bool m_dragWireSegHorizontal = false;
	double m_dragWireOffset = 0.0;
	std::vector<FabricCoord> m_dragWirePath;

	bool m_dragEndpoint = false;
	ObjectId m_dragEndpointWireId{};
	bool m_dragEndpointIsA = false;
	CanvasWire::Endpoint m_dragEndpointOriginal;
	bool m_dragEndpointPortDynamic = false;
	bool m_dragEndpointPortShared = false;
	PortRef m_dragEndpointPort{};
	CanvasPort m_dragEndpointPortMeta{};
	size_t m_dragEndpointPortIndex = 0;

	bool m_pendingEndpoint = false;
	ObjectId m_pendingEndpointWireId{};
	std::optional<PortRef> m_pendingEndpointPort;
	QPointF m_pendingEndpointPressScene;
	QPointF m_pendingEndpointPressView;

	CanvasBlock* m_dragBlock = nullptr;
	QPointF m_dragOffset;
	QPointF m_dragStartTopLeft;
	ObjectId m_selected{};
	bool m_hasSelectedPort = false;
	PortRef m_selectedPort{};

	std::optional<EdgeCandidate> m_hoverEdge;
};

} // namespace Canvas
