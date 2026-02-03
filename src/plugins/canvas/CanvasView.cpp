#include "canvas/CanvasView.hpp"

#include "canvas/CanvasLinkRouteEditor.hpp"

#include <command/BuiltInCommands.hpp>
#include <command/CommandDispatcher.hpp>

#include <QtCore/QDebug>

#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QFontDatabase>
#include <QtGui/QColor>
#include <QtGui/QVector2D>

#include <algorithm>
#include <cmath>

#include <core/widgets/InfoBarWidget.hpp>
#include <core/widgets/StatusBarFieldWidget.hpp>
#include <core/StatusBarField.hpp>
#include <QtWidgets/QLabel>

namespace Canvas {

static void warnRouteEdit(const QString& msg)
{
    qWarning().noquote() << "canvas route edit:" << msg;
}

static QFont canvasFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(10);
    return font;
}

CanvasView::CanvasView(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::StrongFocus);
    rebuildScene();
}

void CanvasView::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    maybeAttachStatusBar();
    syncStatusBar();
}

void CanvasView::setDocument(const DesignModel::DesignDocument& doc)
{
    m_doc = doc;
    rebuildScene();
    syncStatusBar();
    update();
}

void CanvasView::setRenderOptions(const CanvasRenderOptions& opts)
{
    m_opts = opts;
    rebuildScene();
    syncStatusBar();
    update();
}

void CanvasView::rebuildScene()
{
    m_scene.rebuild(m_doc, m_vp, m_opts);
    if (m_hoverHotspot >= m_scene.hotspots().size())
        m_hoverHotspot = -1;
    if (m_hoverBlock >= m_scene.blocks().size())
        m_hoverBlock = -1;
}

void CanvasView::maybeAttachStatusBar()
{
    if (m_statusBar)
        return;

    QWidget* w = window();
    if (!w)
        return;

    auto* bar = w->findChild<Core::InfoBarWidget*>("PlaygroundBottomBar");
    if (!bar)
        return;

    m_statusBar = bar;

    m_fieldMode = m_statusBar->ensureField("mode");
    m_fieldMode->setLabel("MODE");
    m_fieldMode->setSide(Core::StatusBarField::Side::Left);

    m_fieldGrid = m_statusBar->ensureField("grid");
    m_fieldGrid->setLabel("GRID");
    m_fieldGrid->setSide(Core::StatusBarField::Side::Left);

    m_fieldZoom = m_statusBar->ensureField("zoom");
    m_fieldZoom->setLabel("ZOOM");
    m_fieldZoom->setSide(Core::StatusBarField::Side::Left);

    m_fieldPan = m_statusBar->ensureField("pan");
    m_fieldPan->setLabel("PAN");
    m_fieldPan->setSide(Core::StatusBarField::Side::Left);

    syncModePill();
}

void CanvasView::syncModePill()
{
    if (!m_statusBar || !m_fieldMode)
        return;

    const auto widgets = m_statusBar->findChildren<Core::StatusBarFieldWidget*>();
    Core::StatusBarFieldWidget* modeWidget = nullptr;
    for (Core::StatusBarFieldWidget* w : widgets) {
        if (w && w->field() == m_fieldMode) {
            modeWidget = w;
            break;
        }
    }
    if (!modeWidget)
        return;

    QLabel* value = modeWidget->findChild<QLabel*>("StatusBarFieldValue");
    if (!value)
        return;

    QColor bg(80, 80, 80);
    QColor fg(18, 19, 22);

    const bool panning = m_panning || m_spaceDown;
    if (panning) {
        bg = QColor(70, 120, 200);  // blue-ish
        fg = QColor(10, 12, 14);
    } else if (m_mode == EditorModeKind::Linking) {
        bg = QColor(80, 220, 120);  // matches link green
        fg = QColor(10, 12, 14);
    } else {
        bg = QColor(150, 160, 170); // neutral for selection
        fg = QColor(10, 12, 14);
    }

    const QString style = QString(
        "QLabel#StatusBarFieldValue {"
        " background: rgba(%1,%2,%3,0.95);"
        " color: rgba(%4,%5,%6,1.0);"
        " padding: 1px 10px;"
        " font-weight: 700;"
        " }"
    ).arg(bg.red()).arg(bg.green()).arg(bg.blue())
     .arg(fg.red()).arg(fg.green()).arg(fg.blue());

    value->setStyleSheet(style);
}

void CanvasView::syncStatusBar()
{
    maybeAttachStatusBar();
    if (!m_statusBar)
        return;

    if (m_fieldMode) {
        QString mode;
        if (m_panning || m_spaceDown)
            mode = "PAN";
        else
            mode = (m_mode == EditorModeKind::Linking) ? "LINKING" : "NORMAL";
        m_fieldMode->setValue(mode);
    }

    if (m_fieldGrid) {
        const auto& g = m_scene.gridSpec();
        m_fieldGrid->setValue(QString("%1x%2").arg(g.aieCols).arg(g.aieRows));
    }

    if (m_fieldZoom) {
        const int pct = int(std::lround(m_vp.zoomFactor() * 100.0));
        m_fieldZoom->setValue(QString::number(pct) + "%");
    }

    if (m_fieldPan) {
        const QPointF pan = m_vp.pan();
        m_fieldPan->setValue(QString("(%1, %2)")
                                 .arg(int(std::lround(pan.x())))
                                 .arg(int(std::lround(pan.y()))));
    }

    syncModePill();
}

void CanvasView::wheelEvent(QWheelEvent* e)
{
    const int steps = e->angleDelta().y() / 120;
    if (steps != 0) {
        const QPointF cursor = e->position();
        const QPointF world = m_vp.screenToWorld(cursor);
        m_vp.stepZoom(steps);
        const double z = m_vp.zoomFactor();
        m_vp.setPan(QPointF(cursor.x() - world.x() * z,
                            cursor.y() - world.y() * z));
        rebuildScene();
        syncStatusBar();
        update();
    }
    e->accept();
}

void CanvasView::mouseMoveEvent(QMouseEvent* e)
{
    m_mousePos = e->position();

    if (m_panning) {
        const QPointF now = e->position();
        const QPointF delta = now - m_lastMousePos;
        m_lastMousePos = now;
        m_vp.panBy(delta);
        rebuildScene();
        syncStatusBar();
        update();
        e->accept();
        return;
    }

    if (m_routeEditActive && (e->buttons() & Qt::LeftButton)) {
        updateRouteEdit(e->position());
        e->accept();
        return;
    }

    updateHover(e->position());
    if (m_linkArmed)
        update();
    QWidget::mouseMoveEvent(e);
}

void CanvasView::mousePressEvent(QMouseEvent* e)
{
    m_mousePos = e->position();

    const bool wantPan = (e->button() == Qt::MiddleButton)
                      || (e->button() == Qt::LeftButton && m_spaceDown);
    if (wantPan) {
        m_panning = true;
        m_lastMousePos = e->position();
        setCursor(Qt::ClosedHandCursor);
        syncStatusBar();
        e->accept();
        return;
    }

    if (e->button() == Qt::LeftButton) {
        if (m_mode == EditorModeKind::Linking) {
            const int hs = hitHotspot(e->position());
            if (!m_linkArmed) {
                if (hs != -1 && isValidLinkStart(hs)) {
                    m_linkArmed = true;
                    m_linkFromPort = m_scene.hotspots()[hs].portId;
                    m_linkFromPos = m_scene.hotspots()[hs].rect.center();
                    update();
                    e->accept();
                    return;
                }

                if (hs == -1) {
                    if (beginRouteEdit(e->position())) {
                        update();
                        e->accept();
                        return;
                    }
                    DesignModel::LinkId lid;
                    int seg = -1;
                    if (hitLinkSegment(e->position(), &lid, &seg)) {
                        warnRouteEdit(QString("can't start link=%1 seg=%2")
                                         .arg(lid.toString())
                                         .arg(seg));
                    }
                }
            } else {
                if (hs != -1 && isValidLinkTarget(hs, m_linkFromPort)) {
                    (void)applyLink(m_linkFromPort, m_scene.hotspots()[hs].portId);
                    cancelLinkArmed();
                    updateHover(e->position());
                    update();
                    e->accept();
                    return;
                }
                cancelLinkArmed();
                updateHover(e->position());
                update();
                e->accept();
                return;
            }
        }

        const int bi = hitBlock(e->position());
        if (bi != -1) {
            m_selectedBlock = m_scene.blocks()[bi].id;
        } else {
            m_selectedBlock = DesignModel::BlockId{};
        }
        update();
        e->accept();
        return;
    }

    QWidget::mousePressEvent(e);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* e)
{
    m_mousePos = e->position();

    if (m_panning && (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton)) {
        m_panning = false;
        if (m_spaceDown)
            setCursor(Qt::OpenHandCursor);
        else
            unsetCursor();
        syncStatusBar();
        e->accept();
        return;
    }

    if (m_routeEditActive && e->button() == Qt::LeftButton) {
        commitRouteEdit();
        updateHover(e->position());
        update();
        e->accept();
        return;
    }

    QWidget::mouseReleaseEvent(e);
}

void CanvasView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space) {
        if (!m_spaceDown) {
            m_spaceDown = true;
            if (!m_panning)
                setCursor(Qt::OpenHandCursor);
            syncStatusBar();
        }
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_L) {
        setMode(m_mode == EditorModeKind::Linking ? EditorModeKind::Selection : EditorModeKind::Linking);
        e->accept();
        return;
    }

    if (e->key() == Qt::Key_Escape) {
        if (m_routeEditActive) {
            cancelRouteEdit(true);
            e->accept();
            return;
        }
        setMode(EditorModeKind::Selection);
        cancelLinkArmed();
        e->accept();
        return;
    }

    QWidget::keyPressEvent(e);
}

void CanvasView::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space) {
        m_spaceDown = false;
        if (!m_panning)
            unsetCursor();
        syncStatusBar();
        e->accept();
        return;
    }
    QWidget::keyReleaseEvent(e);
}

void CanvasView::leaveEvent(QEvent* e)
{
    Q_UNUSED(e);
    if (m_hoverHotspot != -1 || m_hoverBlock != -1) {
        m_hoverHotspot = -1;
        m_hoverBlock = -1;
        update();
    }
}

void CanvasView::updateHover(const QPointF& p)
{
    const int newHotspot = hitHotspot(p);
    const int newBlock = hitBlock(p);

    if (newHotspot != m_hoverHotspot || newBlock != m_hoverBlock) {
        m_hoverHotspot = newHotspot;
        m_hoverBlock = newBlock;
        update();
    }
}

int CanvasView::hitHotspot(const QPointF& p) const
{
    if (!m_opts.showPortHotspots)
        return -1;
    const auto& hs = m_scene.hotspots();
    for (int i = 0; i < hs.size(); ++i) {
        if (hs[i].rect.contains(p))
            return i;
    }
    return -1;
}

int CanvasView::hitBlock(const QPointF& p) const
{
    const auto& bs = m_scene.blocks();
    for (int i = 0; i < bs.size(); ++i) {
        if (bs[i].rect.contains(p))
            return i;
    }
    return -1;
}

static double distancePointToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const double dx = b.x() - a.x();
    const double dy = b.y() - a.y();
    const double len2 = dx * dx + dy * dy;
    if (len2 < 1e-9) {
        const double ux = p.x() - a.x();
        const double uy = p.y() - a.y();
        return std::sqrt(ux * ux + uy * uy);
    }
    const double t = std::clamp(((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / len2, 0.0, 1.0);
    const QPointF proj(a.x() + t * dx, a.y() + t * dy);
    const double ux = p.x() - proj.x();
    const double uy = p.y() - proj.y();
    return std::sqrt(ux * ux + uy * uy);
}

bool CanvasView::hitLinkSegment(const QPointF& p, DesignModel::LinkId* outLinkId, int* outSegIndex) const
{
    if (outLinkId) *outLinkId = {};
    if (outSegIndex) *outSegIndex = -1;

    const auto& ls = m_scene.links();
    if (ls.isEmpty())
        return false;

    const double threshold = 6.0;
    double best = threshold;
    DesignModel::LinkId bestId;
    int bestSeg = -1;

    for (const auto& lv : ls) {
        const auto& pts = lv.points;
        for (int i = 0; i + 1 < pts.size(); ++i) {
            const double d = distancePointToSegment(p, pts[i], pts[i + 1]);
            if (d < best) {
                best = d;
                bestId = lv.id;
                bestSeg = i;
            }
        }
    }

    if (bestSeg == -1)
        return false;
    if (outLinkId) *outLinkId = bestId;
    if (outSegIndex) *outSegIndex = bestSeg;
    return true;
}


bool CanvasView::extractFabricPath(const QVector<QPointF>& fullWorld,
                                   QVector<QPointF>* outStubAWorld,
                                   QVector<QPointF>* outFabricWorld,
                                   QVector<QPointF>* outStubBWorld) const
{
    if (outStubAWorld) outStubAWorld->clear();
    if (outFabricWorld) outFabricWorld->clear();
    if (outStubBWorld) outStubBWorld->clear();

    if (fullWorld.size() < 2)
        return false;

    const auto& xs = m_scene.fabricXs();
    const auto& ys = m_scene.fabricYs();
    if (xs.isEmpty() || ys.isEmpty())
        return false;

    auto nearIn = [](double v, const QVector<double>& a, double eps) {
        for (double x : a) {
            if (std::abs(v - x) <= eps)
                return true;
        }
        return false;
    };

    auto isFabricNode = [&](const QPointF& p) {
        const double eps = 1e-3;
        return nearIn(p.x(), xs, eps) && nearIn(p.y(), ys, eps);
    };

    int first = -1;
    int last = -1;
    for (int i = 0; i < fullWorld.size(); ++i) {
        if (isFabricNode(fullWorld[i])) {
            first = i;
            break;
        }
    }
    for (int i = fullWorld.size() - 1; i >= 0; --i) {
        if (isFabricNode(fullWorld[i])) {
            last = i;
            break;
        }
    }

    if (first == -1 || last == -1)
        return false;
    if (last - first < 1)
        return false;

    if (outStubAWorld && first > 0)
        *outStubAWorld = fullWorld.mid(0, first + 1);
    if (outFabricWorld)
        *outFabricWorld = fullWorld.mid(first, last - first + 1);
    if (outStubBWorld && last < fullWorld.size() - 1)
        *outStubBWorld = fullWorld.mid(last, fullWorld.size() - last);

    return outFabricWorld && outFabricWorld->size() >= 2;
}

void CanvasView::buildFullFromFabric(const QVector<QPointF>& stubAWorld,
                                     const QVector<QPointF>& fabricWorld,
                                     const QVector<QPointF>& stubBWorld,
                                     QVector<QPointF>* outFullWorld) const
{
    if (!outFullWorld)
        return;

    outFullWorld->clear();

    auto appendNoDup = [&](const QVector<QPointF>& pts) {
        for (const QPointF& p : pts) {
            if (!outFullWorld->isEmpty()) {
                const QPointF& last = outFullWorld->back();
                if (std::abs(last.x() - p.x()) < 1e-6 && std::abs(last.y() - p.y()) < 1e-6)
                    continue;
            }
            outFullWorld->push_back(p);
        }
    };

    appendNoDup(stubAWorld);
    appendNoDup(fabricWorld);
    appendNoDup(stubBWorld);
}

bool CanvasView::beginRouteEdit(const QPointF& mousePosScreen)
{
    if (m_mode != EditorModeKind::Linking)
        return false;
    if (m_linkArmed)
        return false;

    DesignModel::LinkId lid;
    int seg = -1;
    if (!hitLinkSegment(mousePosScreen, &lid, &seg))
        return false;

    const auto& ls = m_scene.links();
    const auto it = std::find_if(ls.begin(), ls.end(), [&](const LinkVisual& v) { return v.id == lid; });
    if (it == ls.end() || it->worldPoints.size() < 2)
        return false;

    m_routeEditActive = true;
    m_routeEditLink = lid;

    m_routeEditStubAWorld.clear();
    m_routeEditStubBWorld.clear();
    m_routeEditBaseFabricWorld.clear();
    if (!extractFabricPath(it->worldPoints, &m_routeEditStubAWorld, &m_routeEditBaseFabricWorld, &m_routeEditStubBWorld)) {
        m_routeEditActive = false;
        m_routeEditLink = {};
        return false;
    }

    const int firstFabricIndex = m_routeEditStubAWorld.isEmpty() ? 0 : (m_routeEditStubAWorld.size() - 1);
    const int fabricSegCount = m_routeEditBaseFabricWorld.size() - 1;
    if (seg < firstFabricIndex || seg >= firstFabricIndex + fabricSegCount) {
        m_routeEditActive = false;
        m_routeEditLink = {};
        return false;
    }

    m_routeEditFabricSegIndex = seg - firstFabricIndex;
    m_routeEditCurrentFabricWorld = m_routeEditBaseFabricWorld;
    m_routeEditInvalidFabricWorld.clear();
    m_routeEditPreviewWorld.clear();
    m_routeEditInvalidPreviewWorld.clear();
    m_routeEditPreviewValid = true;
    m_routeEditLoggedInvalid = false;

    const QPointF a = m_routeEditBaseFabricWorld[m_routeEditFabricSegIndex];
    const QPointF b = m_routeEditBaseFabricWorld[m_routeEditFabricSegIndex + 1];
    const bool horiz = std::abs(a.y() - b.y()) < 1e-6 && std::abs(a.x() - b.x()) > 1e-6;
    m_routeEditLastSnappedCoord = horiz ? a.y() : a.x();
    m_routeEditLastHadLaneDelta = false;

    warnRouteEdit(QString("begin link=%1 seg=%2 pts=%3 xs=%4 ys=%5")
                     .arg(m_routeEditLink.toString())
                     .arg(m_routeEditFabricSegIndex)
                     .arg(m_routeEditBaseFabricWorld.size())
                     .arg(m_scene.fabricXs().size())
                     .arg(m_scene.fabricYs().size()));
    return true;
}

void CanvasView::updateRouteEdit(const QPointF& mousePosScreen)
{
    if (!m_routeEditActive)
        return;

    const QPointF mouseWorld = m_vp.screenToWorld(mousePosScreen);
    const double clearance = 2.0;

    const auto r = LinkRouteEditor::shiftSegmentToNearestLane(
        m_routeEditCurrentFabricWorld,
        m_routeEditFabricSegIndex,
        mouseWorld,
        m_scene.fabricXs(),
        m_scene.fabricYs(),
        m_scene.fabricObstacles(),
        clearance);

    {
        const auto& pts = m_routeEditCurrentFabricWorld;
        const QPointF a = (m_routeEditFabricSegIndex >= 0 && m_routeEditFabricSegIndex < pts.size()) ? pts[m_routeEditFabricSegIndex] : QPointF();
        const QPointF b = (m_routeEditFabricSegIndex + 1 >= 0 && m_routeEditFabricSegIndex + 1 < pts.size()) ? pts[m_routeEditFabricSegIndex + 1] : QPointF();
        const bool horiz = std::abs(a.y() - b.y()) < 1e-6 && std::abs(a.x() - b.x()) > 1e-6;
        const double baseCoord = horiz ? a.y() : a.x();
        const double laneDelta = std::abs(r.snappedCoord - baseCoord);
        if (laneDelta < 1e-6) {
            m_routeEditPreviewValid = true;
            m_routeEditInvalidPreviewWorld.clear();
            m_routeEditLoggedInvalid = false;
            m_routeEditLastHadLaneDelta = false;
            m_scene.clearLinkRoutePreview(m_routeEditLink);
            rebuildScene();
            return;
        }
        m_routeEditLastHadLaneDelta = true;
    }

    if (r.ok) {
        m_routeEditPreviewValid = true;
        m_routeEditInvalidPreviewWorld.clear();
        m_routeEditCurrentFabricWorld = r.worldPoints;
        buildFullFromFabric(m_routeEditStubAWorld, m_routeEditCurrentFabricWorld, m_routeEditStubBWorld, &m_routeEditPreviewWorld);
        m_scene.setLinkRoutePreview(m_routeEditLink, m_routeEditPreviewWorld);
        rebuildScene();
        m_routeEditLoggedInvalid = false;
        m_routeEditLastSnappedCoord = r.snappedCoord;
        updateHover(mousePosScreen);
        update();
        return;
    }

    m_routeEditPreviewValid = false;
    m_routeEditInvalidFabricWorld = r.worldPoints;
    buildFullFromFabric(m_routeEditStubAWorld, m_routeEditInvalidFabricWorld, m_routeEditStubBWorld, &m_routeEditInvalidPreviewWorld);
    const double dSnap = std::abs(r.snappedCoord - m_routeEditLastSnappedCoord);
    if (!m_routeEditLoggedInvalid || dSnap > 1e-6) {
        warnRouteEdit(QString("preview rejected link=%1 seg=%2 snapped=%3")
                         .arg(m_routeEditLink.toString())
                         .arg(m_routeEditFabricSegIndex)
                         .arg(r.snappedCoord, 0, 'f', 3));
        m_routeEditLoggedInvalid = true;
        m_routeEditLastSnappedCoord = r.snappedCoord;
    }
    update();
}

void CanvasView::commitRouteEdit()
{
    if (!m_routeEditActive)
        return;

    m_scene.clearLinkRoutePreview(m_routeEditLink);

    if (!m_routeEditPreviewValid) {
        warnRouteEdit(QString("commit skipped: invalid preview link=%1 seg=%2")
                         .arg(m_routeEditLink.toString())
                         .arg(m_routeEditFabricSegIndex));
    } else if (!m_dispatcher) {
        warnRouteEdit(QString("commit skipped: no dispatcher link=%1")
                         .arg(m_routeEditLink.toString()));
    } else {
        const auto* link = m_doc.tryLink(m_routeEditLink);
        if (!link || !link->isValid()) {
            warnRouteEdit(QString("commit skipped: missing link=%1")
                             .arg(m_routeEditLink.toString()));
        } else if (m_routeEditCurrentFabricWorld.size() < 2) {
            warnRouteEdit(QString("commit skipped: route too short link=%1")
                             .arg(m_routeEditLink.toString()));
        } else if (m_routeEditCurrentFabricWorld == m_routeEditBaseFabricWorld) {
            warnRouteEdit(QString("commit skipped: no change link=%1")
                             .arg(m_routeEditLink.toString()));
        } else {
            std::optional<DesignModel::RouteOverride> oldOv = link->routeOverride();

            QVector<QPointF> mid;
            if (m_routeEditCurrentFabricWorld.size() > 2)
                mid = m_routeEditCurrentFabricWorld.mid(1, m_routeEditCurrentFabricWorld.size() - 2);

            if (mid.isEmpty()) {
                warnRouteEdit(QString("commit skipped: empty override link=%1")
                                 .arg(m_routeEditLink.toString()));
            } else {
                std::optional<DesignModel::RouteOverride> newOv = DesignModel::RouteOverride(std::move(mid), true);
                Command::AdjustLinkRouteCommand cmd(m_routeEditLink, std::move(oldOv), std::move(newOv));
                const auto res = m_dispatcher->apply(cmd);
                if (!res.ok()) {
                    warnRouteEdit(QString("command failed: %1")
                                     .arg(res.error().message()));
                }
            }
        }
    }

    m_routeEditActive = false;
    m_routeEditLink = {};
    m_routeEditFabricSegIndex = -1;
    m_routeEditStubAWorld.clear();
    m_routeEditStubBWorld.clear();
    m_routeEditBaseFabricWorld.clear();
    m_routeEditCurrentFabricWorld.clear();
    m_routeEditInvalidFabricWorld.clear();
    m_routeEditPreviewWorld.clear();
    m_routeEditInvalidPreviewWorld.clear();
    m_routeEditPreviewValid = true;
    m_routeEditLoggedInvalid = false;
    m_routeEditLastSnappedCoord = 0.0;
    m_routeEditLastHadLaneDelta = false;
}

void CanvasView::cancelRouteEdit(bool revert)
{
    if (!m_routeEditActive)
        return;

    Q_UNUSED(revert);
    m_scene.clearLinkRoutePreview(m_routeEditLink);
    rebuildScene();
    update();
    m_routeEditActive = false;
    m_routeEditLink = {};
    m_routeEditFabricSegIndex = -1;
    m_routeEditStubAWorld.clear();
    m_routeEditStubBWorld.clear();
    m_routeEditBaseFabricWorld.clear();
    m_routeEditCurrentFabricWorld.clear();
    m_routeEditInvalidFabricWorld.clear();
    m_routeEditPreviewWorld.clear();
    m_routeEditInvalidPreviewWorld.clear();
    m_routeEditPreviewValid = true;
}

void CanvasView::setMode(EditorModeKind m)
{
    if (m == m_mode)
        return;

    if (m == EditorModeKind::Selection && m_routeEditActive)
        cancelRouteEdit(true);

    m_mode = m;
    cancelLinkArmed();
    syncStatusBar();
    update();
}

void CanvasView::cancelLinkArmed()
{
    if (!m_linkArmed)
        return;
    m_linkArmed = false;
    m_linkFromPort = DesignModel::PortId{};
    m_linkFromPos = QPointF();
}

bool CanvasView::isValidLinkStart(int hotspotIndex) const
{
    if (hotspotIndex < 0 || hotspotIndex >= m_scene.hotspots().size())
        return false;
    const auto& h = m_scene.hotspots()[hotspotIndex];
    if (h.portId.isNull())
        return false;
    const auto* p = m_doc.tryPort(h.portId);
    return p && (p->direction() == DesignModel::PortDirection::Output
              || p->direction() == DesignModel::PortDirection::Input);
}

bool CanvasView::isValidLinkTarget(int hotspotIndex, DesignModel::PortId from) const
{
    if (from.isNull())
        return false;
    if (hotspotIndex < 0 || hotspotIndex >= m_scene.hotspots().size())
        return false;
    const auto& h = m_scene.hotspots()[hotspotIndex];
    if (h.portId.isNull() || h.portId == from)
        return false;
    const auto* fp = m_doc.tryPort(from);
    const auto* tp = m_doc.tryPort(h.portId);
    if (!fp || !tp)
        return false;
    const auto fd = fp->direction();
    const auto td = tp->direction();
    const bool ok = (fd == DesignModel::PortDirection::Output && td == DesignModel::PortDirection::Input)
                 || (fd == DesignModel::PortDirection::Input && td == DesignModel::PortDirection::Output);
    return ok;
}

bool CanvasView::applyLink(DesignModel::PortId from, DesignModel::PortId to)
{
    if (!m_dispatcher)
        return false;

    const auto* fp = m_doc.tryPort(from);
    const auto* tp = m_doc.tryPort(to);
    if (!fp || !tp)
        return false;

    DesignModel::PortId out = from;
    DesignModel::PortId in = to;
    if (fp->direction() == DesignModel::PortDirection::Input
        && tp->direction() == DesignModel::PortDirection::Output) {
        out = to;
        in = from;
    }

    if (m_doc.tryPort(out)->direction() != DesignModel::PortDirection::Output
        || m_doc.tryPort(in)->direction() != DesignModel::PortDirection::Input) {
        return false;
    }

    Command::CreateLinkCommand cmd(out, in);
    const auto r = m_dispatcher->apply(cmd);
    return r.ok();
}

void CanvasView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(16, 18, 20));

    p.setFont(canvasFont());

    for (const auto& tv : m_scene.tiles()) {
        const QRectF r = tv.rect;
        QColor border(70, 80, 88);
        QColor fill(20, 22, 25);
        if (tv.kind == DesignModel::TileKind::Mem) fill = QColor(18, 22, 28);
        if (tv.kind == DesignModel::TileKind::Shim) fill = QColor(22, 20, 28);

        p.setPen(border);
        p.setBrush(fill);
        p.drawRoundedRect(r, 4.0, 4.0);

        p.setPen(QColor(90, 100, 108));
        const double c = std::min(r.width(), r.height()) * 0.10;
        p.drawLine(r.topLeft(), r.topLeft() + QPointF(c, 0));
        p.drawLine(r.topLeft(), r.topLeft() + QPointF(0, c));
        p.drawLine(r.topRight(), r.topRight() + QPointF(-c, 0));
        p.drawLine(r.topRight(), r.topRight() + QPointF(0, c));
        p.drawLine(r.bottomLeft(), r.bottomLeft() + QPointF(c, 0));
        p.drawLine(r.bottomLeft(), r.bottomLeft() + QPointF(0, -c));
        p.drawLine(r.bottomRight(), r.bottomRight() + QPointF(-c, 0));
        p.drawLine(r.bottomRight(), r.bottomRight() + QPointF(0, -c));

        p.setPen(QColor(170, 180, 190));
        p.drawText(r.adjusted(6, 4, -6, -4), Qt::AlignTop | Qt::AlignLeft, tv.label);
    }

    if (m_opts.showFabric) {
        double tilePx = 70.0;
        if (!m_scene.tiles().isEmpty())
            tilePx = std::min(m_scene.tiles().front().rect.width(), m_scene.tiles().front().rect.height());

        const double nodeRadius = std::clamp(tilePx * 0.02, 1.5, 3.0);

        QPen edgePen(QColor(80, 100, 90, 70));
        edgePen.setWidthF(1.0);
        edgePen.setCapStyle(Qt::RoundCap);
        p.setPen(edgePen);
        p.setBrush(Qt::NoBrush);

        for (const auto& e : m_scene.fabricEdges())
            p.drawLine(e.line);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(90, 120, 105, 90));
        for (const auto& n : m_scene.fabricNodes())
            p.drawEllipse(n.pos, nodeRadius, nodeRadius);
    }

    if (m_opts.showPortHotspots) {
        const auto& hs = m_scene.hotspots();
        for (int i = 0; i < hs.size(); ++i) {
            const bool hovered = (i == m_hoverHotspot);
            const bool hasPort = !hs[i].portId.isNull();

            QColor c(90, 90, 90);
            if (!hasPort)
                c = QColor(55, 55, 55);

            if (m_mode == EditorModeKind::Linking) {
                if (m_linkArmed) {
                    if (hs[i].portId == m_linkFromPort)
                        c = QColor(80, 220, 120);
                    else if (hovered) {
                        c = isValidLinkTarget(i, m_linkFromPort) ? QColor(80, 220, 120) : QColor(220, 90, 90);
                    }
                } else {
                    if (hovered)
                        c = isValidLinkStart(i) ? QColor(80, 220, 120) : QColor(220, 90, 90);
                }
            } else {
                if (hovered)
                    c = QColor(120, 160, 180);
            }

            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(hs[i].rect, 2.0, 2.0);
        }
    }

    {
        const QColor innerColor(80, 220, 120);
        const QColor outlineColor(70, 120, 220);

        const double innerWidth = 2.2;
        const double outlineWidth = innerWidth + 2.0;

        QPen outline(outlineColor);
        outline.setWidthF(outlineWidth);
        outline.setJoinStyle(Qt::RoundJoin);
        outline.setCapStyle(Qt::RoundCap);

        QPen inner(innerColor);
        inner.setWidthF(innerWidth);
        inner.setJoinStyle(Qt::RoundJoin);
        inner.setCapStyle(Qt::RoundCap);

        auto arrowMetrics = [&]() {
            double tilePx = 70.0;
            if (!m_scene.tiles().isEmpty())
                tilePx = std::min(m_scene.tiles().front().rect.width(), m_scene.tiles().front().rect.height());
            const double al = std::clamp(tilePx * 0.12, 8.0, 18.0);
            const double aw = std::clamp(al * 0.45, 4.0, 10.0);
            return std::pair<double,double>(al, aw);
        };

        auto drawArrow = [&](const QPen& pen, QPointF a, QPointF b, double al, double aw) {
            const QVector2D v(b - a);
            if (v.length() < 1e-3)
                return;
            const QVector2D d = v.normalized();
            const QVector2D n(-d.y(), d.x());
            const QPointF tip = b;
            const QPointF base = b - d.toPointF() * al;
            QPolygonF tri;
            tri << tip
                << (base + n.toPointF() * aw)
                << (base - n.toPointF() * aw);
            p.setPen(Qt::NoPen);
            p.setBrush(pen.color());
            p.drawPolygon(tri);
            p.setBrush(Qt::NoBrush);
        };

        const auto [al, aw] = arrowMetrics();

        for (const auto& lv : m_scene.links()) {
            if (lv.points.size() < 2)
                continue;

            p.setPen(outline);
            p.drawPolyline(lv.points.constData(), lv.points.size());
            drawArrow(outline, lv.points[lv.points.size() - 2], lv.points.back(), al + 2.0, aw + 1.0);

            p.setPen(inner);
            p.drawPolyline(lv.points.constData(), lv.points.size());
            drawArrow(inner, lv.points[lv.points.size() - 2], lv.points.back(), al, aw);
        }
    }

    if (m_routeEditActive && !m_routeEditPreviewValid && !m_routeEditInvalidPreviewWorld.isEmpty()) {
        QVector<QPointF> pts;
        pts.reserve(m_routeEditInvalidPreviewWorld.size());
        for (const QPointF& wp : m_routeEditInvalidPreviewWorld)
            pts.push_back(m_vp.worldToScreen(wp));

        QPen pen(QColor(220, 90, 90));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(2.0);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (pts.size() >= 2)
            p.drawPolyline(pts.constData(), pts.size());
    }

    for (const auto& bv : m_scene.blocks()) {
        p.setPen(QColor(200, 200, 200));
        p.setBrush(QColor(30, 35, 40));
        p.drawRoundedRect(bv.rect, 6.0, 6.0);
        p.setPen(QColor(220, 220, 220));
        p.drawText(bv.rect, Qt::AlignCenter, bv.text);
    }

    if (!m_selectedBlock.isNull()) {
        for (const auto& bv : m_scene.blocks()) {
            if (bv.id != m_selectedBlock)
                continue;
            QPen sel(QColor(230, 230, 255));
            sel.setWidthF(2.5);
            p.setPen(sel);
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(bv.rect.adjusted(-2, -2, 2, 2), 8.0, 8.0);
            break;
        }
    }

    if (m_linkArmed) {
        QPointF end = m_mousePos;
        if (m_hoverHotspot != -1 && isValidLinkTarget(m_hoverHotspot, m_linkFromPort))
            end = m_scene.hotspots()[m_hoverHotspot].rect.center();

        QPen pen(QColor(180, 220, 180));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(2.0);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawLine(m_linkFromPos, end);
    }

    if (m_opts.showAnnotations) {
        for (const auto& av : m_scene.annotations()) {
            const QRectF a = av.anchorRect;
            if (a.isEmpty())
                continue;
            QRectF bubble(a.topRight() + QPointF(8, 0), QSizeF(140, 32));
            p.setPen(QColor(120, 120, 120));
            p.setBrush(QColor(24, 24, 24, 220));
            p.drawRoundedRect(bubble, 6.0, 6.0);
            p.setPen(QColor(230, 230, 230));
            p.drawText(bubble.adjusted(8, 6, -8, -6), Qt::AlignLeft | Qt::AlignVCenter, av.text);
        }
    }
}

} // namespace Canvas
