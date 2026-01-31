#pragma once

#include "canvas/CanvasRenderOptions.hpp"
#include "canvas/CanvasSceneModel.hpp"
#include "canvas/CanvasViewport.hpp"

#include <designmodel/DesignDocument.hpp>

#include <QtWidgets/QWidget>

namespace Command { class CommandDispatcher; }
namespace Core { class InfoBarWidget; class StatusBarField; }

namespace Canvas {

enum class EditorModeKind : quint8 { Selection, Linking };

class CanvasView final : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasView(QWidget* parent = nullptr);
    ~CanvasView() override = default;

    void setCommandDispatcher(Command::CommandDispatcher* dispatcher) noexcept { m_dispatcher = dispatcher; }

public slots:
    void setDocument(const DesignModel::DesignDocument& doc);
    void setRenderOptions(const Canvas::CanvasRenderOptions& opts);

protected:
    void paintEvent(QPaintEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void rebuildScene();

    void maybeAttachStatusBar();
    void syncStatusBar();
    void syncModePill();
    void updateHover(const QPointF& p);
    int hitHotspot(const QPointF& p) const;
    int hitBlock(const QPointF& p) const;
    void setMode(EditorModeKind m);
    void cancelLinkArmed();

    bool beginRouteEdit(const QPointF& mousePosScreen);
    void updateRouteEdit(const QPointF& mousePosScreen);
    void commitRouteEdit();
    void cancelRouteEdit(bool revert);
    bool isEditingRoute() const noexcept { return m_routeEditActive; }
    bool hitLinkSegment(const QPointF& p, DesignModel::LinkId* outLinkId, int* outSegIndex) const;

    bool extractFabricPath(const QVector<QPointF>& fullWorld,
                           QVector<QPointF>* outStubAWorld,
                           QVector<QPointF>* outFabricWorld,
                           QVector<QPointF>* outStubBWorld) const;

    void buildFullFromFabric(const QVector<QPointF>& stubAWorld,
                             const QVector<QPointF>& fabricWorld,
                             const QVector<QPointF>& stubBWorld,
                             QVector<QPointF>* outFullWorld) const;

    bool isValidLinkStart(int hotspotIndex) const;
    bool isValidLinkTarget(int hotspotIndex, DesignModel::PortId from) const;
    bool applyLink(DesignModel::PortId from, DesignModel::PortId to);

    DesignModel::DesignDocument m_doc;
    CanvasRenderOptions m_opts;
    CanvasViewport m_vp;
    CanvasSceneModel m_scene;

    Command::CommandDispatcher* m_dispatcher{nullptr}; // not owned

    EditorModeKind m_mode{EditorModeKind::Selection};
    bool m_spaceDown{false};
    bool m_panning{false};
    QPointF m_lastMousePos;

    DesignModel::BlockId m_selectedBlock{};

    int m_hoverHotspot{-1};
    int m_hoverBlock{-1};

    bool m_linkArmed{false};
    DesignModel::PortId m_linkFromPort{};
    QPointF m_linkFromPos;
    QPointF m_mousePos;

    bool m_routeEditActive{false};
    DesignModel::LinkId m_routeEditLink{};
    int m_routeEditFabricSegIndex{-1};
    QVector<QPointF> m_routeEditStubAWorld;
    QVector<QPointF> m_routeEditStubBWorld;

    QVector<QPointF> m_routeEditBaseFabricWorld;
    QVector<QPointF> m_routeEditCurrentFabricWorld;
    QVector<QPointF> m_routeEditInvalidFabricWorld;

    QVector<QPointF> m_routeEditPreviewWorld;
    QVector<QPointF> m_routeEditInvalidPreviewWorld;
    bool m_routeEditPreviewValid{true};
    bool m_routeEditLoggedInvalid{false};
    double m_routeEditLastSnappedCoord{0.0};
    bool m_routeEditLastHadLaneDelta{false};

    Core::InfoBarWidget* m_statusBar{nullptr};   // not owned
    Core::StatusBarField* m_fieldMode{nullptr};  // not owned
    Core::StatusBarField* m_fieldGrid{nullptr};  // not owned
    Core::StatusBarField* m_fieldZoom{nullptr};  // not owned
    Core::StatusBarField* m_fieldPan{nullptr};   // not owned
};

} // namespace Canvas
