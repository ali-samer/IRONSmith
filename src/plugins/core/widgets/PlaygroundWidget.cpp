#include "core/widgets/PlaygroundWidget.hpp"

#include <algorithm>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QSizePolicy>

#include "core/StatusBarField.hpp"
#include "core/ui/UiStyle.hpp"
#include "core/widgets/InfoBarWidget.hpp"

namespace Core {

class ResizableSidebarContainer;

class SidebarPanelSlot final : public QWidget
{
public:
    explicit SidebarPanelSlot(const char* objectName, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(objectName);
        setAttribute(Qt::WA_StyledBackground, true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setMinimumWidth(0);

        m_installHost = new QWidget(this);
        m_installHost->setObjectName("SidebarPanelInstallHost");
        m_installHost->setAttribute(Qt::WA_StyledBackground, false);
        m_installHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        l->addWidget(m_installHost, 1);

        auto* installLayout = new QVBoxLayout(m_installHost);
        installLayout->setContentsMargins(0, 0, 0, 0);
        installLayout->setSpacing(0);
    }

    QWidget* installHost() const { return m_installHost; }

private:
    QWidget* m_installHost = nullptr;
};

class SidebarResizeGrip final : public QWidget
{
public:
    enum class Side { Left, Right };

    SidebarResizeGrip(ResizableSidebarContainer* owner, Side side, QWidget* parent = nullptr)
        : QWidget(parent), m_owner(owner), m_side(side)
    {
        setObjectName("SidebarResizeGrip");
        setFixedWidth(kGripPx);
        setCursor(Qt::SplitHCursor);
        setMouseTracking(true);
        setAttribute(Qt::WA_StyledBackground, false);
    }

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    static constexpr int kGripPx = 6;

    ResizableSidebarContainer* m_owner = nullptr; // non-owning
    Side m_side;
    bool m_resizing = false;
    qreal m_pressGlobalX = 0.0;
    int m_pressContentW = 0;
};

class ResizableSidebarContainer final : public QWidget
{
public:
    enum class Side { Left, Right };

    ResizableSidebarContainer(Side side, int defaultContentW, int minContentW, int maxContentW, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_side(side)
        , m_defaultContentW(defaultContentW)
        , m_minContentW(minContentW)
        , m_maxContentW(maxContentW)
        , m_savedContentW(defaultContentW)
    {
        setObjectName(m_side == Side::Left ? "LeftSidebarHost" : "RightSidebarHost");
        setAttribute(Qt::WA_StyledBackground, true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        m_installHost = new QWidget(this);
        m_installHost->setObjectName("SidebarInstallHost");
        m_installHost->setAttribute(Qt::WA_StyledBackground, false);
        m_installHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto* installLayout = new QVBoxLayout(m_installHost);
        installLayout->setContentsMargins(0, 0, 0, 0);
        installLayout->setSpacing(0);

        auto* root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        if (m_side == Side::Left) {
            root->addWidget(m_installHost, 1);
            m_grip = new SidebarResizeGrip(this, SidebarResizeGrip::Side::Left, this);
            root->addWidget(m_grip, 0);
        } else {
            m_grip = new SidebarResizeGrip(this, SidebarResizeGrip::Side::Right, this);
            root->addWidget(m_grip, 0);
            root->addWidget(m_installHost, 1);
        }

        m_installHost->installEventFilter(this);

        setVisible(false);
        setFixedWidth(0);
    }

    QWidget* installHost() const { return m_installHost; }

    int contentWidth() const noexcept { return m_savedContentW; }

    void setContentWidthClamped(int w)
    {
        w = std::clamp(w, m_minContentW, m_maxContentW);
        m_savedContentW = w;
        applyVisibleWidthFromSaved();
    }

    bool hasInstalledRail() const
    {
        const auto kids = m_installHost->children();
        for (QObject* obj : kids) {
            if (qobject_cast<QWidget*>(obj))
                return true;
        }
        return false;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* e) override
    {
        if (watched == m_installHost) {
            if (e->type() == QEvent::ChildAdded || e->type() == QEvent::ChildRemoved) {
                syncCollapsedState();
            }
        }
        return QWidget::eventFilter(watched, e);
    }

private:
    friend class SidebarResizeGrip;

    void syncCollapsedState()
    {
        if (hasInstalledRail()) {
            setVisible(true);
            applyVisibleWidthFromSaved();
        } else {
            setVisible(false);
            setFixedWidth(0);
        }
    }

    void applyVisibleWidthFromSaved()
    {
        if (!hasInstalledRail())
            return;

        const int totalW = m_savedContentW + m_grip->width();
        setFixedWidth(totalW);
    }

private:
    Side m_side;
    int m_defaultContentW;
    int m_minContentW;
    int m_maxContentW;
    int m_savedContentW;

    QWidget* m_installHost = nullptr;
    SidebarResizeGrip* m_grip = nullptr;
};

void SidebarResizeGrip::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton || !m_owner || !m_owner->hasInstalledRail()) {
        QWidget::mousePressEvent(e);
        return;
    }

    m_resizing = true;
    m_pressGlobalX = e->globalPosition().x();
    m_pressContentW = m_owner->contentWidth();
    grabMouse();
    e->accept();
}

void SidebarResizeGrip::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_resizing || !m_owner) {
        QWidget::mouseMoveEvent(e);
        return;
    }

    const int dx = int(e->globalPosition().x() - m_pressGlobalX);

    int newW = m_pressContentW;
    if (m_side == Side::Left)
        newW += dx;
    else
        newW -= dx;

    m_owner->setContentWidthClamped(newW);
    e->accept();
}

void SidebarResizeGrip::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_resizing && e->button() == Qt::LeftButton) {
        m_resizing = false;
        releaseMouse();
        e->accept();
        return;
    }
    QWidget::mouseReleaseEvent(e);
}

PlaygroundWidget::PlaygroundWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("PlaygroundRoot");
    setAttribute(Qt::WA_StyledBackground, true);

    m_rootStack = new QStackedLayout(this);
    m_rootStack->setContentsMargins(0, 0, 0, 0);
    m_rootStack->setStackingMode(QStackedLayout::StackAll);

    m_baseHost = new QWidget(this);
    m_baseHost->setObjectName("BaseHost");
    m_baseHost->setAttribute(Qt::WA_StyledBackground, true);
    m_rootStack->addWidget(m_baseHost);

    m_chromeOverlay = new QWidget(this);
    m_chromeOverlay->setObjectName("PlaygroundChromeOverlay");
    m_chromeOverlay->setAttribute(Qt::WA_StyledBackground, false);

    m_chromeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_rootStack->addWidget(m_chromeOverlay);
    m_rootStack->setCurrentWidget(m_chromeOverlay);

    auto* chromeRoot = new QVBoxLayout(m_chromeOverlay);
    chromeRoot->setContentsMargins(0, 0, 0, 0);
    chromeRoot->setSpacing(0);

    m_topBar = new InfoBarWidget(m_chromeOverlay);
    m_topBar->setObjectName("PlaygroundTopBar");
    m_topBar->setFixedHeight(Ui::UiStyle::TopBarHeight);
    m_topBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chromeRoot->addWidget(m_topBar, 0);

    auto* middle = new QWidget(m_chromeOverlay);
    middle->setObjectName("PlaygroundMiddle");
    middle->setAttribute(Qt::WA_StyledBackground, false);

    middle->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    auto* midLayout = new QHBoxLayout(middle);
    midLayout->setContentsMargins(0, 0, 0, 0);
    midLayout->setSpacing(0);

    constexpr int kRailDefault = Ui::UiStyle::SidebarWidth; // e.g. 44
    constexpr int kRailMin = 36;
    constexpr int kRailMax = 96;

    auto* left = new ResizableSidebarContainer(ResizableSidebarContainer::Side::Left,
                                               kRailDefault, kRailMin, kRailMax, middle);
    left->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_leftSidebarContainer = left;
    m_leftSidebarInstallHost = left->installHost();
    midLayout->addWidget(left, 0);

    auto* leftPanelSlot = new SidebarPanelSlot("LeftSidebarPanelSlot", middle);
    leftPanelSlot->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_leftSidebarPanelInstallHost = leftPanelSlot->installHost();
    midLayout->addWidget(leftPanelSlot, 0);

    auto* centerSpacer = new QWidget(middle);
    centerSpacer->setObjectName("PlaygroundCenterSpacer");
    centerSpacer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    centerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    midLayout->addWidget(centerSpacer, 1);

    auto* rightPanelSlot = new SidebarPanelSlot("RightSidebarPanelSlot", middle);
    rightPanelSlot->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_rightSidebarPanelInstallHost = rightPanelSlot->installHost();
    midLayout->addWidget(rightPanelSlot, 0);

    auto* right = new ResizableSidebarContainer(ResizableSidebarContainer::Side::Right,
                                                kRailDefault, kRailMin, kRailMax, middle);
    right->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_rightSidebarContainer = right;
    m_rightSidebarInstallHost = right->installHost();
    midLayout->addWidget(right, 0);

    chromeRoot->addWidget(middle, 1);

    m_bottomBar = new InfoBarWidget(m_chromeOverlay);
    m_bottomBar->setObjectName("PlaygroundBottomBar");
    m_bottomBar->setFixedHeight(Ui::UiStyle::BottomBarHeight);
    m_bottomBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chromeRoot->addWidget(m_bottomBar, 0);

    // TODO: remove code below when ready to display global state
    // Testing purposes
    auto* mode = m_bottomBar->ensureField("mode");
    mode->setLabel("MODE");
    mode->setValue("VIEW");
    mode->setSide(StatusBarField::Side::Left);

    auto* grid = m_bottomBar->ensureField("grid");
    grid->setLabel("GRID");
    grid->setValue("4x4");
    grid->setSide(StatusBarField::Side::Left);

    auto* zoom = m_bottomBar->ensureField("zoom");
    zoom->setLabel("ZOOM");
    zoom->setValue("100%");
    zoom->setSide(StatusBarField::Side::Left);

    auto* pan = m_bottomBar->ensureField("pan");
    pan->setLabel("PAN");
    pan->setValue("(0, 0)");
    pan->setSide(StatusBarField::Side::Left);
}

InfoBarWidget* PlaygroundWidget::topBar() const { return m_topBar; }
InfoBarWidget* PlaygroundWidget::bottomBar() const { return m_bottomBar; }

QWidget* PlaygroundWidget::leftSidebarHost() const { return m_leftSidebarInstallHost; }
QWidget* PlaygroundWidget::rightSidebarHost() const { return m_rightSidebarInstallHost; }

QWidget* PlaygroundWidget::leftSidebarPanelHost() const { return m_leftSidebarPanelInstallHost; }
QWidget* PlaygroundWidget::rightSidebarPanelHost() const { return m_rightSidebarPanelInstallHost; }

QWidget* PlaygroundWidget::centerBaseHost() const { return m_baseHost; }
QWidget* PlaygroundWidget::overlayHost() const { return m_chromeOverlay; }

} // namespace Core