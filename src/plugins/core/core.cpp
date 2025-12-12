#include "core.hpp"

#include "PluginManager.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPlainTextEdit>
#include <QtCore/QEvent>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QTreeView>
#include <QtCore/QTimer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QtWidgets/QFileDialog>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QItemSelection>
#include <QtCore/QJsonDocument>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtGui/QIcon>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtGui/QBrush>
#include <QtGui/QDrag>
#include <QtGui/QCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QFocusEvent>
#include <QtCore/QMimeData>
#include <QtCore/QSize>
#include <QtCore/QPoint>
#include <QtCore/QSignalBlocker>
#include <QtCore/QList>
#include <QtWidgets/QSlider>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidgetAction>
#include <optional>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <QStringList>
#include <utility>
#include <vector>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QCoreApplication>
#include <QtNodes/internal/ConnectionIdHash.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/DefaultConnectionPainter.hpp>

#include <QtCore/QDebug>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/Definitions>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QRadioButton>

#include <unordered_set>

using namespace QtNodes;

namespace aiecad {

// Allow loop connections without altering the underlying node editor library.
class LoopingGraphModel : public DataFlowGraphModel
{
public:
    using DataFlowGraphModel::DataFlowGraphModel;
    bool loopsEnabled() const override { return true; }

    bool setPortData(NodeId nodeId, PortType portType, PortIndex portIndex,
                     QVariant const &value, PortRole role) override
    {
        if (role == PortRole::Data && portType == PortType::In) {
            if (m_inPortSetGuard)
                return false; // prevent re-entrant self-loop propagation
            m_inPortSetGuard = true;
            const bool res = DataFlowGraphModel::setPortData(nodeId, portType, portIndex, value, role);
            m_inPortSetGuard = false;
            return res;
        }
        return DataFlowGraphModel::setPortData(nodeId, portType, portIndex, value, role);
    }

private:
    bool m_inPortSetGuard { false };
};

// -------------------------------------------------------------
// Simple numeric data type for the demo nodes
// -------------------------------------------------------------

class NumberData : public NodeData
{
public:
    NumberData() = default;
    explicit NumberData(double v) : m_value(v) {}

    NodeDataType type() const override
    {
        return { QStringLiteral("number"), QStringLiteral("Number") };
    }

    double value() const { return m_value; }
    void setValue(double v) { m_value = v; }

private:
    double m_value {0.0};
};

// Delegate to provide $variable autocompletion in dimension cells
class SymbolDimsDelegate : public QStyledItemDelegate
{
public:
    using CompletionProvider = std::function<QStringList()>;

    explicit SymbolDimsDelegate(CompletionProvider provider, QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_provider(std::move(provider))
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        if (auto *line = qobject_cast<QLineEdit *>(editor)) {
            line->setPlaceholderText(QObject::tr("e.g. 4, 8, $N"));
            if (m_provider) {
                const QStringList completions = m_provider();
                auto *completer = new QCompleter(completions, line);
                completer->setCaseSensitivity(Qt::CaseInsensitive);
                completer->setFilterMode(Qt::MatchContains);
                line->setCompleter(completer);
                QObject::connect(line, &QLineEdit::textEdited, line,
                                 [completer, line](const QString &text) {
                                     const int dollar = text.lastIndexOf(QLatin1Char('$'));
                                     if (dollar >= 0) {
                                         completer->setCompletionPrefix(text.mid(dollar));
                                         completer->complete(line->rect());
                                     }
                                 });
            }
        }
        return editor;
    }

private:
    CompletionProvider m_provider;
};

class StickyComboBox : public QComboBox
{
public:
    explicit StickyComboBox(QWidget *parent = nullptr)
        : QComboBox(parent)
    {
        if (view())
            view()->installEventFilter(this);
    }

    void forceHide()
    {
        m_allowHide = true;
        QComboBox::hidePopup();
    }

protected:
    void hidePopup() override
    {
        if (!m_allowHide)
            return;
        m_allowHide = false;
        QComboBox::hidePopup();
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == view() && event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (ke->key() == Qt::Key_Escape) {
                forceHide();
                return true;
            }
        }
        return QComboBox::eventFilter(watched, event);
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        m_allowHide = true;
        QComboBox::hidePopup();
        QComboBox::focusOutEvent(event);
    }

private:
    bool m_allowHide {false};
};

// -------------------------------------------------------------
// Base helper for our three node kinds
// -------------------------------------------------------------

class BaseNodeModel : public NodeDelegateModel
{
public:
    BaseNodeModel(QString name, QString caption,
                  unsigned int inPorts, unsigned int outPorts,
                  QString description = {})
        : m_name(std::move(name))
        , m_caption(std::move(caption))
        , m_inCount(inPorts)
        , m_outCount(outPorts)
        , m_description(std::move(description))
    {}

    QString name() const override { return m_name; }
    QString caption() const override { return m_caption; }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(PortType portType) const override
    {
        if (portType == PortType::In)
            return m_inCount;
        if (portType == PortType::Out)
            return m_outCount;
        return 0U;
    }

    NodeDataType dataType(PortType, PortIndex) const override
    {
        // All ports carry NumberData in this demo.
        return { QStringLiteral("number"), QStringLiteral("Number") };
    }

    std::shared_ptr<NodeData> outData(PortIndex index) override
    {
        Q_UNUSED(index);
        return m_number;
    }

    void setInData(std::shared_ptr<NodeData> data,
                   PortIndex index) override
    {
        Q_UNUSED(index);
        auto number = std::dynamic_pointer_cast<NumberData>(data);
        if (number)
            m_number = number;

        // In a real app we would recompute here and emit dataUpdated().
        Q_EMIT dataUpdated(0);
    }

    QWidget *embeddedWidget() override
    {
        return nullptr; // no inline widget for now
    }

    bool portCaptionVisible(PortType, PortIndex) const override { return true; }

    QString portCaption(PortType portType, PortIndex index) const override
    {
        return portType == PortType::In
                   ? QStringLiteral("In %1").arg(index)
                   : QStringLiteral("Out %1").arg(index);
    }

    bool resizable() const override { return true; }
    QString description() const { return m_description; }
    void setDescription(const QString &desc) { m_description = desc; }
    QString customName() const { return m_customName.isEmpty() ? m_caption : m_customName; }
    void setCustomName(const QString &n) { m_customName = n; }

    void addInPort()
    {
        Q_EMIT portsAboutToBeInserted(PortType::In, m_inCount, m_inCount);
        ++m_inCount;
        Q_EMIT portsInserted();
        Q_EMIT dataUpdated(0);
    }

    void addOutPort()
    {
        Q_EMIT portsAboutToBeInserted(PortType::Out, m_outCount, m_outCount);
        ++m_outCount;
        Q_EMIT portsInserted();
        Q_EMIT dataUpdated(0);
    }

    void removeInPort()
    {
        if (m_inCount == 0)
            return;

        const PortIndex last = m_inCount - 1;
        Q_EMIT portsAboutToBeDeleted(PortType::In, last, last);
        --m_inCount;
        Q_EMIT portsDeleted();
        Q_EMIT dataUpdated(0);
    }

    void removeOutPort()
    {
        if (m_outCount == 0)
            return;

        const PortIndex last = m_outCount - 1;
        Q_EMIT portsAboutToBeDeleted(PortType::Out, last, last);
        --m_outCount;
        Q_EMIT portsDeleted();
        Q_EMIT dataUpdated(0);
    }

private:
    QString m_name;
    QString m_caption;
    QString m_description;
    QString m_customName;
    unsigned int m_inCount {0};
    unsigned int m_outCount {0};

protected:
    std::shared_ptr<NumberData> m_number { std::make_shared<NumberData>(0.0) };
};

// Additional palette node models
class KernelNodeModel : public BaseNodeModel
{
public:
    KernelNodeModel()
        : BaseNodeModel(QStringLiteral("Kernel"),
                        QStringLiteral("Kernel"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class BufferNodeModel : public BaseNodeModel
{
public:
    BufferNodeModel()
        : BaseNodeModel(QStringLiteral("Buffer"),
                        QStringLiteral("Buffer"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class ConstantNodeModel : public BaseNodeModel
{
public:
    ConstantNodeModel()
        : BaseNodeModel(QStringLiteral("Constant"),
                        QStringLiteral("Constant"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class PortNodeModel : public BaseNodeModel
{
public:
    PortNodeModel()
        : BaseNodeModel(QStringLiteral("Port"),
                        QStringLiteral("Port"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class TileNodeModel : public BaseNodeModel
{
public:
    TileNodeModel()
        : BaseNodeModel(QStringLiteral("Tile"),
                        QStringLiteral("Tile"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class ShimTileNodeModel : public BaseNodeModel
{
public:
    ShimTileNodeModel()
        : BaseNodeModel(QStringLiteral("ShimTile"),
                        QStringLiteral("Shim"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class MemoryTileNodeModel : public BaseNodeModel
{
public:
    MemoryTileNodeModel()
        : BaseNodeModel(QStringLiteral("MemoryTile"),
                        QStringLiteral("Memory"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class StreamSplitNodeModel : public BaseNodeModel
{
public:
    StreamSplitNodeModel()
        : BaseNodeModel(QStringLiteral("StreamSplit"),
                        QStringLiteral("Stream Split"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class StreamJoinNodeModel : public BaseNodeModel
{
public:
    StreamJoinNodeModel()
        : BaseNodeModel(QStringLiteral("StreamJoin"),
                        QStringLiteral("Stream Join"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class StreamThroughNodeModel : public BaseNodeModel
{
public:
    StreamThroughNodeModel()
        : BaseNodeModel(QStringLiteral("StreamThrough"),
                        QStringLiteral("Stream Through"),
                        /*in*/ 1,
                        /*out*/ 1,
                        QStringLiteral("Pass a stream through unchanged."))
    {}
};

class GroupNodeModel : public BaseNodeModel
{
public:
    GroupNodeModel()
        : BaseNodeModel(QStringLiteral("Group"),
                        QStringLiteral("Group"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

// Draggable toolbox list
class ToolboxListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit ToolboxListWidget(QWidget *parent = nullptr)
        : QListWidget(parent)
    {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setDefaultDropAction(Qt::IgnoreAction);
    }

protected:
    void startDrag(Qt::DropActions) override
    {
        if (auto *item = currentItem()) {
            auto *mime = new QMimeData;
            mime->setData("application/x-aiecad-node-type", item->text().toUtf8());

            auto *drag = new QDrag(this);
            drag->setMimeData(mime);
            drag->exec(Qt::CopyAction);
        }
    }
};

// -------------------------------------------------------------
// Three visual node types: Entry / Compute / Output
// -------------------------------------------------------------

class EntryNodeModel : public BaseNodeModel
{
public:
    EntryNodeModel()
        : BaseNodeModel(QStringLiteral("EntryNode"),
                        QStringLiteral("Entry"),
                        /*in*/ 0,
                        /*out*/ 1)
    {}
};

class ComputeNodeModel : public BaseNodeModel
{
public:
    ComputeNodeModel()
        : BaseNodeModel(QStringLiteral("ComputeNode"),
                        QStringLiteral("Compute"),
                        /*in*/ 1,
                        /*out*/ 1)
    {}
};

class OutputNodeModel : public BaseNodeModel
{
public:
    OutputNodeModel()
        : BaseNodeModel(QStringLiteral("OutputNode"),
                        QStringLiteral("Output"),
                        /*in*/ 1,
                        /*out*/ 0)
    {}
};

// -------------------------------------------------------------
// NodeEditorWidget: central widget hosting QtNodes view
// -------------------------------------------------------------

class NodeEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NodeEditorWidget(QWidget *parent = nullptr);

    DataFlowGraphModel *graphModel() const { return m_graphModel; }
    DataFlowGraphicsScene *scene() const { return m_scene; }
    GraphicsView *view() const { return m_view; }
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void addEntryNode();
    void addShimNode();
    void addMemoryNode();
    void addComputeNode();
    void addOutputNode();
    void setNpuVersion(const QString &version);
    QString npuVersionString() const;

    bool canAddNode(const QString &typeId) const;
    int countNodesOfType(const QString &typeId) const;
    int maxAllowedForType(const QString &typeId) const;
    void showLimitMessage(const QString &typeId) const;
    bool enforceNodeLimit(NodeId nodeId);

    void deleteSelectedItems();

private:
    QPointF takeNextNodePosition();
    NodeId createNodeAt(const QString &typeId, const QPointF &pos);
    void removeNode(NodeId nodeId);
    void removeConnection(const ConnectionId &connectionId);
    void copySelection();
    void pasteSelection();
    void setEdgeAnimationEnabled(bool enabled);
    void setNodeMovementLocked(bool locked);
    struct PortConstraints;
    PortConstraints constraintsFor(BaseNodeModel *base) const;
    void ensureMinimumPorts(BaseNodeModel *base);
    void captureSpacingBaseline();
    void applySpacingFromSliders();
    void updateSpacingControls();

    std::shared_ptr<NodeDelegateModelRegistry> m_registry;
    DataFlowGraphModel *m_graphModel { nullptr };
    DataFlowGraphicsScene *m_scene { nullptr };
    GraphicsView *m_view { nullptr };

    int m_row {0};
    int m_column {0};

    QJsonObject m_copyBuffer;
    bool m_hasCopy { false };
    StickyComboBox *m_canvasProps { nullptr };
    QTimer *m_edgeAnimTimer { nullptr };
    bool m_animateEdges { false };
    QToolButton *m_spacingButton { nullptr };
    QCheckBox *m_lockNodesCheckbox { nullptr };
    QSlider *m_spacingCombined { nullptr };
    QSlider *m_spacingHorizontal { nullptr };
    QSlider *m_spacingVertical { nullptr };
    std::unordered_map<NodeId, QPointF> m_spacingBaseline;
    bool m_nodesLocked { false };

    enum class NpuVersion { V1, V2 };
    NpuVersion m_npuVersion { NpuVersion::V2 };

Q_SIGNALS:
    void npuVersionChanged(const QString &version);
    void nodeCreatedWithPosition(NodeId nodeId);
    void generateCodeRequested();
};

NodeEditorWidget::NodeEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar with buttons
    auto *toolbarWidget = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(8, 8, 8, 8);
    toolbarLayout->setSpacing(10);

    auto *addTileButton = new QToolButton(toolbarWidget);
    addTileButton->setText(tr("Add Tile"));
    addTileButton->setPopupMode(QToolButton::InstantPopup);
    addTileButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addTileButton->setAutoRaise(true);
    addTileButton->setIconSize(QSize(18, 18));
    QFont buttonFont = addTileButton->font();
    buttonFont.setPointSize(buttonFont.pointSize() + 1);
    addTileButton->setFont(buttonFont);
    const QIcon addIcon = QIcon::fromTheme(QStringLiteral("list-add"),
                                           addTileButton->style()->standardIcon(QStyle::SP_DialogYesButton));
    addTileButton->setIcon(addIcon);
    addTileButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; width: 0px; }"));

    auto *addMenu = new QMenu(addTileButton);
    addMenu->addSection(tr("Tile Types"));
    QAction *addEntryAction  = addMenu->addAction(addIcon, tr("Add Entry"));
    QAction *addShimAction   = addMenu->addAction(addIcon, tr("Add Shim"));
    QAction *addMemoryAction = addMenu->addAction(addIcon, tr("Add Memory"));
    QAction *addComputeAction = addMenu->addAction(addIcon, tr("Add Compute"));
    QAction *addOutputAction = addMenu->addAction(addIcon, tr("Add Output"));
    addTileButton->setMenu(addMenu);

    toolbarLayout->addWidget(addTileButton);

    auto *canvasPropsBtn = new QToolButton(toolbarWidget);
    canvasPropsBtn->setText(tr("Canvas Properties"));
    canvasPropsBtn->setPopupMode(QToolButton::InstantPopup);
    canvasPropsBtn->setAutoRaise(true);
    canvasPropsBtn->setFont(buttonFont);
    auto *canvasMenu = new QMenu(canvasPropsBtn);
    auto *animateAction = canvasMenu->addAction(tr("Animate Edge Direction"));
    animateAction->setCheckable(true);
    canvasPropsBtn->setMenu(canvasMenu);
    toolbarLayout->addWidget(canvasPropsBtn);

    auto *spacingBtn = new QToolButton(toolbarWidget);
    spacingBtn->setText(tr("Configurations"));
    spacingBtn->setPopupMode(QToolButton::InstantPopup);
    spacingBtn->setAutoRaise(true);
    spacingBtn->setFont(buttonFont);
        auto *spacingMenu = new QMenu(spacingBtn);
        auto *spacingWidget = new QWidget(spacingMenu);
        auto *spacingLayout = new QVBoxLayout(spacingWidget);
        spacingLayout->setContentsMargins(8, 8, 8, 8);
        spacingLayout->setSpacing(6);

        m_lockNodesCheckbox = new QCheckBox(tr("Lock Placement"), spacingWidget);
        spacingLayout->addWidget(m_lockNodesCheckbox);
        connect(m_lockNodesCheckbox, &QCheckBox::toggled, this, [this](bool locked) {
            setNodeMovementLocked(locked);
        });

    auto makeSlider = [this, spacingWidget, spacingLayout](const QString &label, QSlider *&slider) {
        auto *lbl = new QLabel(label, spacingWidget);
        slider = new QSlider(Qt::Horizontal, spacingWidget);
        slider->setRange(50, 200); // percentage
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setTickInterval(25);
        slider->setValue(100);
        slider->setEnabled(false);
        spacingLayout->addWidget(lbl);
        spacingLayout->addWidget(slider);
        connect(slider, &QSlider::valueChanged, this, &NodeEditorWidget::applySpacingFromSliders);
    };

    makeSlider(tr("Combined"), m_spacingCombined);
    makeSlider(tr("Horizontal"), m_spacingHorizontal);
    makeSlider(tr("Vertical"), m_spacingVertical);

    auto *spacingAction = new QWidgetAction(spacingMenu);
    spacingAction->setDefaultWidget(spacingWidget);
    spacingMenu->addAction(spacingAction);
    spacingBtn->setMenu(spacingMenu);
    toolbarLayout->addWidget(spacingBtn);
    m_spacingButton = spacingBtn;

    toolbarLayout->addStretch(1);
    auto *generateButton = new QToolButton(toolbarWidget);
    generateButton->setText(tr("Generate Code"));
    generateButton->setAutoRaise(true);
    generateButton->setFont(buttonFont);
    generateButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  padding: 6px 12px;"
        "  border-radius: 6px;"
        "  background: #2f73ff;"
        "  color: white;"
        "}"
        "QToolButton:hover { background: #1f5ed6; }"
        "QToolButton:pressed { background: #174cb0; }"));
    toolbarLayout->addWidget(generateButton);

    layout->addWidget(toolbarWidget);

    // Registry and graph model
    m_registry = std::make_shared<NodeDelegateModelRegistry>();
    m_registry->registerModel<EntryNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<ComputeNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<OutputNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<KernelNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<BufferNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<ConstantNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<PortNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<TileNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<ShimTileNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<MemoryTileNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<StreamSplitNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<StreamJoinNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<StreamThroughNodeModel>(QStringLiteral("AIECAD"));
    m_registry->registerModel<GroupNodeModel>(QStringLiteral("AIECAD"));

    m_graphModel = new LoopingGraphModel(m_registry);
    m_graphModel->setParent(this);

    m_scene = new DataFlowGraphicsScene(*m_graphModel);
    m_scene->setParent(this);
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &NodeEditorWidget::captureSpacingBaseline);

    connect(m_graphModel, &DataFlowGraphModel::nodeCreated,
            this, [this](NodeId nodeId) {
                if (!enforceNodeLimit(nodeId))
                    return;
                if (m_nodesLocked)
                    setNodeMovementLocked(true);
            });

    m_view = new GraphicsView(m_scene);

    layout->addWidget(m_view, 1);

    connect(addEntryAction,  &QAction::triggered,
            this, &NodeEditorWidget::addEntryNode);
    connect(addShimAction,   &QAction::triggered,
            this, &NodeEditorWidget::addShimNode);
    connect(addMemoryAction, &QAction::triggered,
            this, &NodeEditorWidget::addMemoryNode);
    connect(addComputeAction, &QAction::triggered,
            this, &NodeEditorWidget::addComputeNode);
    connect(addOutputAction,  &QAction::triggered,
            this, &NodeEditorWidget::addOutputNode);
    connect(animateAction, &QAction::toggled, this, [this](bool checked) {
        setEdgeAnimationEnabled(checked);
    });
    connect(generateButton, &QToolButton::clicked, this, [this]() {
        Q_EMIT generateCodeRequested();
    });

    // Handle keyboard shortcuts via event filter to avoid ambiguous QAction shortcuts.
    m_view->installEventFilter(this);
    updateSpacingControls();
    setNodeMovementLocked(false);
}

QString NodeEditorWidget::npuVersionString() const
{
    return m_npuVersion == NpuVersion::V1 ? QStringLiteral("npu1") : QStringLiteral("npu2");
}

void NodeEditorWidget::setNpuVersion(const QString &version)
{
    const NpuVersion previous = m_npuVersion;
    const QString lower = version.toLower();
    if (lower == QLatin1String("npu1"))
        m_npuVersion = NpuVersion::V1;
    else if (lower == QLatin1String("npu2"))
        m_npuVersion = NpuVersion::V2;

    if (m_npuVersion != previous)
        Q_EMIT npuVersionChanged(npuVersionString());
}

void NodeEditorWidget::addEntryNode()
{
    createNodeAt(QStringLiteral("EntryNode"), takeNextNodePosition());
}

void NodeEditorWidget::addShimNode()
{
    if (canAddNode(QStringLiteral("ShimTile")))
        createNodeAt(QStringLiteral("ShimTile"), takeNextNodePosition());
    else
        showLimitMessage(QStringLiteral("ShimTile"));
}

void NodeEditorWidget::addMemoryNode()
{
    if (canAddNode(QStringLiteral("MemoryTile")))
        createNodeAt(QStringLiteral("MemoryTile"), takeNextNodePosition());
    else
        showLimitMessage(QStringLiteral("MemoryTile"));
}

void NodeEditorWidget::addComputeNode()
{
    if (canAddNode(QStringLiteral("ComputeNode")))
        createNodeAt(QStringLiteral("ComputeNode"), takeNextNodePosition());
    else
        showLimitMessage(QStringLiteral("ComputeNode"));
}

void NodeEditorWidget::addOutputNode()
{
    createNodeAt(QStringLiteral("OutputNode"), takeNextNodePosition());
}

void NodeEditorWidget::deleteSelectedItems()
{
    if (!m_scene || !m_graphModel)
        return;

    std::unordered_set<NodeId> nodeIds;
    std::unordered_set<ConnectionId> connectionIds;

    const auto selected = m_scene->selectedItems();
    for (auto *item : selected) {
        if (auto *conn = qgraphicsitem_cast<ConnectionGraphicsObject *>(item)) {
            connectionIds.insert(conn->connectionId());
        } else if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
            nodeIds.insert(ngo->nodeId());
        }
    }

    if (nodeIds.empty() && connectionIds.empty())
        return;

    for (const auto &cid : connectionIds)
        removeConnection(cid);

    for (const auto nid : nodeIds)
        removeNode(nid);
}

QPointF NodeEditorWidget::takeNextNodePosition()
{
    const QPointF pos(m_column * 220.0, m_row * 120.0);
    ++m_row;
    if (m_row > 4) {
        m_row = 0;
        ++m_column;
    }
    return pos;
}

NodeId NodeEditorWidget::createNodeAt(const QString &typeId, const QPointF &pos)
{
    if (!m_graphModel)
        return InvalidNodeId;

    NodeId id = m_graphModel->addNode(typeId);
    m_graphModel->setNodeData(id, NodeRole::Position, pos);
    if (m_scene) {
        if (auto *ngo = m_scene->nodeGraphicsObject(id)) {
            m_scene->clearSelection();
            ngo->setSelected(true);
        }
    }
    Q_EMIT nodeCreatedWithPosition(id);
    return id;
}

void NodeEditorWidget::removeNode(NodeId nodeId)
{
    if (m_graphModel && m_graphModel->nodeExists(nodeId))
        m_graphModel->deleteNode(nodeId);
}

void NodeEditorWidget::removeConnection(const ConnectionId &connectionId)
{
    if (m_graphModel && m_graphModel->connectionExists(connectionId))
        m_graphModel->deleteConnection(connectionId);
}

int NodeEditorWidget::countNodesOfType(const QString &typeId) const
{
    if (!m_graphModel)
        return 0;

    int count = 0;
    for (auto nid : m_graphModel->allNodeIds()) {
        auto *delegate = m_graphModel->delegateModel<NodeDelegateModel>(nid);
        if (delegate && delegate->name() == typeId)
            ++count;
    }
    return count;
}

bool NodeEditorWidget::canAddNode(const QString &typeId) const
{
    const int maxAllowed = maxAllowedForType(typeId);
    if (maxAllowed < 0)
        return true;
    return countNodesOfType(typeId) < maxAllowed;
}

int NodeEditorWidget::maxAllowedForType(const QString &typeId) const
{
    if (typeId == QLatin1String("ComputeNode"))
        return m_npuVersion == NpuVersion::V1 ? 16 : 32;
    if (typeId == QLatin1String("MemoryTile") || typeId == QLatin1String("ShimTile"))
        return m_npuVersion == NpuVersion::V1 ? 4 : 8;
    return -1;
}

void NodeEditorWidget::showLimitMessage(const QString &typeId) const
{
    const int maxAllowed = maxAllowedForType(typeId);
    if (maxAllowed < 0)
        return;

    QString typeLabel;
    if (typeId == QLatin1String("ComputeNode"))
        typeLabel = tr("Compute");
    else if (typeId == QLatin1String("MemoryTile"))
        typeLabel = tr("Memory");
    else if (typeId == QLatin1String("ShimTile"))
        typeLabel = tr("Shim");
    else
        typeLabel = typeId;

    const QString version = m_npuVersion == NpuVersion::V1 ? tr("NPU v1") : tr("NPU v2");
    QString extra;
    if (m_npuVersion == NpuVersion::V1)
        extra = tr("Switch to NPU v2 to add more.");

    QMessageBox::information(nullptr,
                             tr("Tile Limit Reached"),
                             tr("%1 allows up to %2 %3 tiles. %4").arg(version)
                             .arg(maxAllowed).arg(typeLabel).arg(extra));
}

bool NodeEditorWidget::enforceNodeLimit(NodeId nodeId)
{
    if (!m_graphModel)
        return true;

    auto *delegate = m_graphModel->delegateModel<NodeDelegateModel>(nodeId);
    if (!delegate)
        return true;

    const QString typeId = delegate->name();
    const int maxAllowed = maxAllowedForType(typeId);
    if (maxAllowed < 0)
        return true;

    const int count = countNodesOfType(typeId);
    if (count > maxAllowed) {
        showLimitMessage(typeId);
        m_graphModel->deleteNode(nodeId);
        return false;
    }
    return true;
}

void NodeEditorWidget::copySelection()
{
    if (!m_scene || !m_graphModel)
        return;

    QJsonArray nodesArray;
    QJsonArray connectionsArray;

    std::unordered_set<NodeId> selectedNodes;

    const auto items = m_scene->selectedItems();
    for (auto *item : items) {
        if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item))
            selectedNodes.insert(ngo->nodeId());
    }

    if (selectedNodes.empty())
        return;

    for (NodeId nodeId : selectedNodes) {
        auto *delegate = m_graphModel->delegateModel<NodeDelegateModel>(nodeId);
        if (!delegate)
            continue;

        QJsonObject nodeJson;
        nodeJson["id"] = static_cast<qint64>(nodeId);

        nodeJson["internal-data"] = delegate->save();

        QPointF const pos = m_graphModel->nodeData(nodeId, NodeRole::Position).value<QPointF>();
        QJsonObject posJson;
        posJson["x"] = pos.x();
        posJson["y"] = pos.y();
        nodeJson["position"] = posJson;

        nodesArray.append(nodeJson);
    }

    // Gather connections between selected nodes
    std::unordered_set<ConnectionId> connections;
    for (NodeId nodeId : selectedNodes) {
        auto allForNode = m_graphModel->allConnectionIds(nodeId);
        connections.insert(allForNode.begin(), allForNode.end());
    }

    for (const auto &cid : connections) {
        if (selectedNodes.count(cid.outNodeId) && selectedNodes.count(cid.inNodeId)) {
            connectionsArray.append(toJson(cid));
        }
    }

    QJsonObject buffer;
    buffer["nodes"] = nodesArray;
    buffer["connections"] = connectionsArray;

    m_copyBuffer = buffer;
    m_hasCopy = true;
}

void NodeEditorWidget::pasteSelection()
{
    if (!m_scene || !m_graphModel || !m_hasCopy)
        return;

    QJsonArray nodesArray = m_copyBuffer["nodes"].toArray();
    QJsonArray connectionsArray = m_copyBuffer["connections"].toArray();

    if (nodesArray.isEmpty())
        return;

    constexpr QPointF pasteOffset(40.0, 40.0);
    std::unordered_map<NodeId, NodeId> idMap;

    for (const auto &nodeVal : nodesArray) {
        QJsonObject nodeJson = nodeVal.toObject();
        const NodeId oldId = static_cast<NodeId>(nodeJson["id"].toInt());
        QJsonObject internal = nodeJson["internal-data"].toObject();
        const QString modelName = internal["model-name"].toString();

        NodeId newId = m_graphModel->addNode(modelName);

        QJsonObject posJson = nodeJson["position"].toObject();
        QPointF const pos(posJson["x"].toDouble(), posJson["y"].toDouble());
        m_graphModel->setNodeData(newId, NodeRole::Position, pos + pasteOffset);

        if (auto *delegate = m_graphModel->delegateModel<NodeDelegateModel>(newId)) {
            delegate->load(internal);
        }

        idMap[oldId] = newId;
    }

    for (const auto &connVal : connectionsArray) {
        QJsonObject connJson = connVal.toObject();
        ConnectionId oldCid = fromJson(connJson);
        auto outIt = idMap.find(oldCid.outNodeId);
        auto inIt  = idMap.find(oldCid.inNodeId);
        if (outIt == idMap.end() || inIt == idMap.end())
            continue;

        ConnectionId newCid {
            outIt->second,
            oldCid.outPortIndex,
            inIt->second,
            oldCid.inPortIndex
        };

        m_graphModel->addConnection(newCid);
    }

    // Select newly pasted nodes
    m_scene->clearSelection();
    for (const auto &pair : idMap) {
        if (auto *ngo = m_scene->nodeGraphicsObject(pair.second))
            ngo->setSelected(true);
    }
}

void NodeEditorWidget::setEdgeAnimationEnabled(bool enabled)
{
    if (m_animateEdges == enabled && (!enabled || (m_edgeAnimTimer && m_edgeAnimTimer->isActive())))
        return;

    m_animateEdges = enabled;
    QtNodes::setConnectionAnimationEnabled(enabled);

    if (enabled) {
        if (!m_edgeAnimTimer) {
            m_edgeAnimTimer = new QTimer(this);
            m_edgeAnimTimer->setTimerType(Qt::PreciseTimer);
            m_edgeAnimTimer->setInterval(16); // ~60 FPS for smoother pulses
            connect(m_edgeAnimTimer, &QTimer::timeout, this, [this]() {
                QtNodes::advanceConnectionAnimationPhase(0.015);
                if (m_scene)
                    m_scene->update();
            });
        }
        m_edgeAnimTimer->start();
    } else if (m_edgeAnimTimer) {
        m_edgeAnimTimer->stop();
    }

    if (m_scene)
        m_scene->update();
}

void NodeEditorWidget::setNodeMovementLocked(bool locked)
{
    m_nodesLocked = locked;

    if (m_lockNodesCheckbox) {
        QSignalBlocker blocker(m_lockNodesCheckbox);
        m_lockNodesCheckbox->setChecked(locked);
    }

    if (m_scene)
        m_scene->setMovementLocked(locked);

    if (!m_scene || !m_graphModel)
        return;

    const auto nodeIds = m_graphModel->allNodeIds();
    for (auto id : nodeIds) {
        if (auto *ngo = m_scene->nodeGraphicsObject(id)) {
            ngo->setFlag(QGraphicsItem::ItemIsMovable, !locked);
        }
    }
}

void NodeEditorWidget::captureSpacingBaseline()
{
    m_spacingBaseline.clear();

    if (!m_scene || !m_graphModel) {
        updateSpacingControls();
        return;
    }

    const auto items = m_scene->selectedItems();
    for (auto *item : items) {
        if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
            const NodeId id = ngo->nodeId();
            QPointF pos = m_graphModel->nodeData(id, NodeRole::Position).value<QPointF>();
            if (pos.isNull())
                pos = ngo->pos();
            m_spacingBaseline.emplace(id, pos);
        }
    }

    auto resetSlider = [](QSlider *slider) {
        if (!slider)
            return;
        QSignalBlocker blocker(slider);
        slider->setValue(100);
    };

    resetSlider(m_spacingCombined);
    resetSlider(m_spacingHorizontal);
    resetSlider(m_spacingVertical);

    updateSpacingControls();
}

void NodeEditorWidget::updateSpacingControls()
{
    const bool hasSelection = !m_spacingBaseline.empty();

    if (m_spacingButton)
        m_spacingButton->setEnabled(hasSelection);

    if (m_spacingCombined)
        m_spacingCombined->setEnabled(hasSelection);
    if (m_spacingHorizontal)
        m_spacingHorizontal->setEnabled(hasSelection);
    if (m_spacingVertical)
        m_spacingVertical->setEnabled(hasSelection);
}

void NodeEditorWidget::applySpacingFromSliders()
{
    if (m_spacingBaseline.empty() || !m_graphModel || !m_scene)
        return;

    const double combined = m_spacingCombined ? m_spacingCombined->value() / 100.0 : 1.0;
    const double hFactor = combined * (m_spacingHorizontal ? m_spacingHorizontal->value() / 100.0 : 1.0);
    const double vFactor = combined * (m_spacingVertical ? m_spacingVertical->value() / 100.0 : 1.0);

    QPointF centroid(0.0, 0.0);
    for (const auto &entry : m_spacingBaseline)
        centroid += entry.second;
    centroid /= static_cast<double>(m_spacingBaseline.size());

    for (const auto &entry : m_spacingBaseline) {
        const NodeId id = entry.first;
        if (!m_graphModel->nodeExists(id))
            continue;
        const QPointF base = entry.second;
        const QPointF delta = base - centroid;
        const QPointF newPos(centroid.x() + delta.x() * hFactor,
                             centroid.y() + delta.y() * vFactor);
        m_graphModel->setNodeData(id, NodeRole::Position, newPos);
        if (auto *ngo = m_scene->nodeGraphicsObject(id)) {
            ngo->setPos(newPos);
            ngo->moveConnections();
        }
    }

    m_scene->update();
}


bool NodeEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const bool ctrl = mouseEvent->modifiers() & Qt::ControlModifier;
        if (ctrl && mouseEvent->button() == Qt::LeftButton && m_scene && m_view) {
            QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
            if (auto *item = m_scene->itemAt(scenePos, m_view->transform())) {
                if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
                    ngo->setSelected(!ngo->isSelected());
                    return true;
                }
            }
        }
    }

    if (watched == m_view && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const bool ctrl = keyEvent->modifiers() & Qt::ControlModifier;
        if (!ctrl)
            return QWidget::eventFilter(watched, event);

        if (keyEvent->key() == Qt::Key_Z) {
            return true; // no undo/redo implemented
        }

        if (keyEvent->key() == Qt::Key_Backspace) {
            deleteSelectedItems();
            return true;
        }

        if (keyEvent->key() == Qt::Key_C) {
            copySelection();
            return true;
        }

        if (keyEvent->key() == Qt::Key_V) {
            pasteSelection();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// -------------------------------------------------------------
// CoreMainWindow: top-level IDE shell for the demo
// -------------------------------------------------------------

class CoreMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CoreMainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle(QStringLiteral("AIECAD"));

        auto *central = new QWidget(this);
        auto *centralLayout = new QVBoxLayout(central);
        centralLayout->setContentsMargins(0, 0, 0, 0);

        // Dockable sections: Functions, Modules, Toolbox
        auto *functionsDock = new QDockWidget(tr("Functions"), this);
        functionsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        auto *functionsList  = new QListWidget(functionsDock);
        functionsList->addItem(tr("main"));
        functionsList->addItem(tr("helper"));
        functionsDock->setWidget(functionsList);
        addDockWidget(Qt::LeftDockWidgetArea, functionsDock);

        auto *modulesDock = new QDockWidget(tr("Modules"), this);
        modulesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        auto *modulesList    = new QListWidget(modulesDock);
        modulesList->addItem(tr("math"));
        modulesList->addItem(tr("tensor"));
        modulesList->addItem(tr("aie_graph"));
        modulesDock->setWidget(modulesList);
        addDockWidget(Qt::LeftDockWidgetArea, modulesDock);
        tabifyDockWidget(functionsDock, modulesDock);

        auto *toolboxDock = new QDockWidget(tr("Toolbox"), this);
        toolboxDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        auto *toolboxList  = new ToolboxListWidget(toolboxDock);
        QStringList paletteItems {
            QStringLiteral("Kernel"),
            QStringLiteral("Buffer"),
            QStringLiteral("Constant"),
            QStringLiteral("Port"),
            QStringLiteral("Tile"),
            QStringLiteral("ShimTile"),
            QStringLiteral("MemoryTile"),
            QStringLiteral("StreamSplit"),
            QStringLiteral("StreamJoin"),
            QStringLiteral("StreamThrough"),
            QStringLiteral("Group")
        };
        toolboxList->addItems(paletteItems);
        toolboxDock->setWidget(toolboxList);
        addDockWidget(Qt::LeftDockWidgetArea, toolboxDock);
        tabifyDockWidget(modulesDock, toolboxDock);

        // Symbol Definition panel (Variables / Types)
        auto *symbolDock = new QDockWidget(tr("Symbol Definition"), this);
        symbolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        auto *symbolWidget = new QWidget(symbolDock);
        auto *symbolLayout = new QVBoxLayout(symbolWidget);
        symbolLayout->setContentsMargins(6, 6, 6, 6);
        symbolLayout->setSpacing(6);

        auto *symbolTabs = new QTabWidget(symbolWidget);

        // Variables tab
        auto *varsTab = new QWidget(symbolTabs);
        auto *varsLayout = new QVBoxLayout(varsTab);
        varsLayout->setContentsMargins(0, 0, 0, 0);
        varsLayout->setSpacing(6);

        m_symbolVarTable = new QTableWidget(1, 2, varsTab);
        m_symbolVarTable->setHorizontalHeaderLabels({ tr("Name"), tr("Value") });
        m_symbolVarTable->horizontalHeader()->setStretchLastSection(true);
        m_symbolVarTable->verticalHeader()->setVisible(false);
        m_symbolVarTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_symbolVarTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_symbolVarTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_symbolVarTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("N")));
        m_symbolVarTable->setItem(0, 1, new QTableWidgetItem(QStringLiteral("256")));

        varsLayout->addWidget(m_symbolVarTable, 1);

        auto *varsButtons = new QHBoxLayout;
        varsButtons->setContentsMargins(0, 0, 0, 0);
        varsButtons->addStretch(1);
        auto *removeVarBtn = new QToolButton(varsTab);
        removeVarBtn->setText(QStringLiteral("-"));
        removeVarBtn->setAutoRaise(true);
        auto *addVarBtn = new QToolButton(varsTab);
        addVarBtn->setText(QStringLiteral("+"));
        addVarBtn->setAutoRaise(true);
        varsButtons->addWidget(removeVarBtn);
        varsButtons->addWidget(addVarBtn);
        varsLayout->addLayout(varsButtons);

        connect(addVarBtn, &QToolButton::clicked, this, &CoreMainWindow::addSymbolVariableRow);
        connect(removeVarBtn, &QToolButton::clicked, this, &CoreMainWindow::removeSymbolVariableRow);
        connect(m_symbolVarTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *) {
            updateTypeDimsValidation();
            if (!m_loadingMetadata)
                saveActiveDesignMetadata();
        });

        varsTab->setLayout(varsLayout);
        symbolTabs->addTab(varsTab, tr("Variable"));

        // Types tab (placeholder for future expansion)
        auto *typesTab = new QWidget(symbolTabs);
        auto *typesLayout = new QVBoxLayout(typesTab);
        typesLayout->setContentsMargins(0, 0, 0, 0);
        typesLayout->setSpacing(6);

        m_symbolTypeTable = new QTableWidget(1, 3, typesTab);
        m_symbolTypeTable->setHorizontalHeaderLabels({ tr("Name"), tr("Dimensions"), tr("Type") });
        m_symbolTypeTable->horizontalHeader()->setStretchLastSection(true);
        m_symbolTypeTable->verticalHeader()->setVisible(false);
        m_symbolTypeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_symbolTypeTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_symbolTypeTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_symbolTypeTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("vector_ty")));
        m_symbolTypeTable->setItem(0, 1, new QTableWidgetItem(QStringLiteral("$N")));
        m_symbolTypeTable->setItemDelegateForColumn(1, new SymbolDimsDelegate([this]() {
            return symbolVariableCompletions();
        }, m_symbolTypeTable));
        attachTypeCombo(0, QStringLiteral("int32"));
        m_symbolTypeTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_symbolTypeTable, &QTableWidget::customContextMenuRequested,
                this, &CoreMainWindow::showTypeContextMenu);

        typesLayout->addWidget(m_symbolTypeTable, 1);

        auto *typesButtons = new QHBoxLayout;
        typesButtons->setContentsMargins(0, 0, 0, 0);
        typesButtons->addStretch(1);
        auto *removeTypeBtn = new QToolButton(typesTab);
        removeTypeBtn->setText(QStringLiteral("-"));
        removeTypeBtn->setAutoRaise(true);
        auto *addTypeBtn = new QToolButton(typesTab);
        addTypeBtn->setText(QStringLiteral("+"));
        addTypeBtn->setAutoRaise(true);
        typesButtons->addWidget(removeTypeBtn);
        typesButtons->addWidget(addTypeBtn);
        typesLayout->addLayout(typesButtons);

        connect(addTypeBtn, &QToolButton::clicked, this, &CoreMainWindow::addSymbolTypeRow);
        connect(removeTypeBtn, &QToolButton::clicked, this, &CoreMainWindow::removeSymbolTypeRow);
        connect(m_symbolTypeTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
            if (!item)
                return;

            if (item->column() == 1)
                updateTypeDimsValidation();
            if (item->column() == 0)
                updateFifoTypeOptions();

            refreshTypeDefaultTooltips();
            if (!m_loadingMetadata)
                saveActiveDesignMetadata();
        });
        updateTypeDimsValidation();
        refreshTypeDefaultTooltips();
        updateFifoTypeOptions();

        typesTab->setLayout(typesLayout);
        symbolTabs->addTab(typesTab, tr("Type"));

        symbolLayout->addWidget(symbolTabs);
        symbolWidget->setLayout(symbolLayout);
        symbolDock->setWidget(symbolWidget);
        addDockWidget(Qt::LeftDockWidgetArea, symbolDock);
        tabifyDockWidget(toolboxDock, symbolDock);

        // Design & Dispatch panel
        auto *designDock = new QDockWidget(tr("Design & Dispatch"), this);
        designDock->setAllowedAreas(Qt::AllDockWidgetAreas);
        auto *designSplitter = new QSplitter(Qt::Vertical, designDock);

        auto *designTop = new QWidget(designSplitter);
        auto *designTopLayout = new QVBoxLayout(designTop);
        designTopLayout->setContentsMargins(8, 8, 8, 8);
        auto *designToolbar = new QWidget(designTop);
        auto *designToolbarLayout = new QHBoxLayout(designToolbar);
        designToolbarLayout->setContentsMargins(0, 0, 0, 0);
        designToolbarLayout->setSpacing(6);
        auto *designAddBtn = new QToolButton(designToolbar);
        designAddBtn->setText(QStringLiteral("+"));
        designAddBtn->setAutoRaise(true);
        auto *designRemoveBtn = new QToolButton(designToolbar);
        designRemoveBtn->setText(QStringLiteral("-"));
        designRemoveBtn->setAutoRaise(true);
        designToolbarLayout->addWidget(designAddBtn);
        designToolbarLayout->addWidget(designRemoveBtn);
        auto *designOpenBtn = new QToolButton(designToolbar);
        designOpenBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
        designOpenBtn->setAutoRaise(true);
        designToolbarLayout->addWidget(designOpenBtn);
        auto *designRefreshBtn = new QToolButton(designToolbar);
        designRefreshBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        designRefreshBtn->setAutoRaise(true);
        designToolbarLayout->addWidget(designRefreshBtn);
        m_designPathLabel = new QLabel(tr("No folder selected"), designToolbar);
        m_designPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_designPathLabel->setWordWrap(false);
        m_designPathLabel->setMinimumWidth(0);
        m_designPathLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        designToolbarLayout->addWidget(m_designPathLabel, 1);
        designTopLayout->addWidget(designToolbar);

        m_designModel = new QStandardItemModel(this);
        m_designModel->setHorizontalHeaderLabels({ tr("Design Explorer") });

        m_designTree = new QTreeView(designTop);
        m_designTree->setModel(m_designModel);
        m_designTree->setRootIsDecorated(true);
        m_designTree->setUniformRowHeights(true);
        m_designTree->setHeaderHidden(false);
        m_designTree->setIndentation(18);
        designTopLayout->addWidget(m_designTree, 1);

        connect(designAddBtn, &QToolButton::clicked, this, &CoreMainWindow::addDesignNode);
        connect(designRemoveBtn, &QToolButton::clicked, this, &CoreMainWindow::removeSelectedDesignRoot);
        connect(designOpenBtn, &QToolButton::clicked, this, &CoreMainWindow::openDesignFolder);
        connect(designRefreshBtn, &QToolButton::clicked, this, [this]() {
            if (!m_designRootPath.isEmpty())
                loadDesignFolder(m_designRootPath);
        });
        connect(m_designTree->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &CoreMainWindow::onDesignSelectionChanged);
        connect(m_designTree, &QTreeView::doubleClicked,
                this, &CoreMainWindow::onDesignDoubleClicked);

        auto *designBottom = new QWidget(designSplitter);
        auto *designBottomLayout = new QVBoxLayout(designBottom);
        designBottomLayout->setContentsMargins(8, 8, 8, 8);
        designBottomLayout->addWidget(new QLabel(tr("Dispatch Hub"), designBottom));

        designSplitter->addWidget(designTop);
        designSplitter->addWidget(designBottom);
        designSplitter->setStretchFactor(0, 1);
        designSplitter->setStretchFactor(1, 1);

        designDock->setWidget(designSplitter);
        addDockWidget(Qt::LeftDockWidgetArea, designDock);

        // Right side: properties inspector
        auto *propertiesDock = new QDockWidget(tr("Properties"), this);
        propertiesDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        auto *propertiesWidget = new QWidget(propertiesDock);
        auto *propertiesLayout = new QVBoxLayout(propertiesWidget);
        propertiesLayout->setContentsMargins(8, 8, 8, 8);
        propertiesLayout->setSpacing(6);

        auto *propertiesForm = new QFormLayout;
        propertiesForm->setContentsMargins(0, 0, 0, 0);
        propertiesForm->setSpacing(6);

        m_nameEdit = new QLineEdit(propertiesWidget);
        m_nameEdit->setPlaceholderText(tr("No selection"));
        connect(m_nameEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
            if (m_selectedNode && m_editor && m_editor->graphModel()) {
                if (auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(*m_selectedNode)) {
                    if (auto *base = dynamic_cast<BaseNodeModel *>(delegate)) {
                        base->setCustomName(text);
                        if (!m_loadingMetadata)
                            saveActiveDesignMetadata();
                    }
                }
            }
        });

        m_typeEdit = new QLineEdit(propertiesWidget);
        m_typeEdit->setReadOnly(true);

        m_descEdit = new QPlainTextEdit(propertiesWidget);
        m_descEdit->setReadOnly(true);
        m_descEdit->setPlaceholderText(tr("Select a node to see properties."));
        m_descEdit->setMinimumHeight(140);

        // Ports control row
        auto *portsRowWidget = new QWidget(propertiesWidget);
        auto *portsLayout = new QHBoxLayout(portsRowWidget);
        portsLayout->setContentsMargins(0, 0, 0, 0);
        portsLayout->setSpacing(6);

        auto *inLabel = new QLabel(tr("IN"), portsRowWidget);
        m_inMinus = new QToolButton(portsRowWidget);
        m_inMinus->setText(QStringLiteral("-"));
        m_inMinus->setAutoRaise(true);
        m_inPlus = new QToolButton(portsRowWidget);
        m_inPlus->setText(QStringLiteral("+"));
        m_inPlus->setAutoRaise(true);
        m_inCountLabel = new QLabel(QStringLiteral("0"), portsRowWidget);

        auto *outLabel = new QLabel(tr("OUT"), portsRowWidget);
        m_outMinus = new QToolButton(portsRowWidget);
        m_outMinus->setText(QStringLiteral("-"));
        m_outMinus->setAutoRaise(true);
        m_outPlus = new QToolButton(portsRowWidget);
        m_outPlus->setText(QStringLiteral("+"));
        m_outPlus->setAutoRaise(true);
        m_outCountLabel = new QLabel(QStringLiteral("0"), portsRowWidget);

        portsLayout->addWidget(inLabel);
        portsLayout->addWidget(m_inMinus);
        portsLayout->addWidget(m_inCountLabel);
        portsLayout->addWidget(m_inPlus);
        portsLayout->addSpacing(12);
        portsLayout->addWidget(outLabel);
        portsLayout->addWidget(m_outMinus);
        portsLayout->addWidget(m_outCountLabel);
        portsLayout->addWidget(m_outPlus);
        portsLayout->addStretch(1);

        connect(m_inMinus, &QToolButton::clicked, this, &CoreMainWindow::onRemoveInPort);
        connect(m_inPlus, &QToolButton::clicked, this, &CoreMainWindow::onAddInPort);
        connect(m_outMinus, &QToolButton::clicked, this, &CoreMainWindow::onRemoveOutPort);
        connect(m_outPlus, &QToolButton::clicked, this, &CoreMainWindow::onAddOutPort);
        setPortControlsEnabled(false);
        refreshPortCounts(nullptr);

        m_kernelCombo = new QComboBox(propertiesWidget);
        m_kernelCombo->addItem(tr("None"));
        m_kernelCombo->setEnabled(false);

        propertiesForm->addRow(tr("Name"), m_nameEdit);
        propertiesForm->addRow(tr("Type"), m_typeEdit);
        m_coordLabel = new QLabel(propertiesWidget);
        m_coordLabel->setText(QStringLiteral("-"));
        propertiesForm->addRow(tr("Coordinate"), m_coordLabel);
        propertiesForm->addRow(tr("Ports"), portsRowWidget);
        propertiesForm->addRow(tr("Kernel"), m_kernelCombo);
        propertiesLayout->addLayout(propertiesForm);
        propertiesLayout->addWidget(m_descEdit, 1);

        // Entry-specific Fill overview
        m_entryFillGroup = new QGroupBox(tr("Entry Fills"), propertiesWidget);
        auto *fillOuterLayout = new QVBoxLayout(m_entryFillGroup);
        fillOuterLayout->setContentsMargins(8, 8, 8, 8);
        fillOuterLayout->setSpacing(4);

        auto *fillHeader = new QWidget(m_entryFillGroup);
        auto *fillHeaderLayout = new QHBoxLayout(fillHeader);
        fillHeaderLayout->setContentsMargins(0, 0, 0, 0);
        fillHeaderLayout->setSpacing(6);
        auto makeHeader = [](const QString &text) {
            auto *lbl = new QLabel(text);
            QFont f = lbl->font();
            f.setBold(true);
            lbl->setFont(f);
            return lbl;
        };
        fillHeaderLayout->addWidget(makeHeader(tr("Fill")));
        fillHeaderLayout->addWidget(makeHeader(tr("FIFO")));
        fillHeaderLayout->addWidget(makeHeader(tr("Type")));
        fillHeaderLayout->addWidget(makeHeader(tr("Depth")));
        fillHeaderLayout->addStretch(1);
        m_entryFillLayout = new QVBoxLayout;
        m_entryFillLayout->setContentsMargins(0, 0, 0, 0);
        m_entryFillLayout->setSpacing(4);

        fillOuterLayout->addWidget(fillHeader);
        fillOuterLayout->addLayout(m_entryFillLayout);
        m_entryFillGroup->setVisible(false);

        propertiesLayout->addWidget(m_entryFillGroup);

        // Output-specific Drain overview
        m_outputDrainGroup = new QGroupBox(tr("Output Drains"), propertiesWidget);
        auto *drainOuterLayout = new QVBoxLayout(m_outputDrainGroup);
        drainOuterLayout->setContentsMargins(8, 8, 8, 8);
        drainOuterLayout->setSpacing(4);

        auto *drainHeader = new QWidget(m_outputDrainGroup);
        auto *drainHeaderLayout = new QHBoxLayout(drainHeader);
        drainHeaderLayout->setContentsMargins(0, 0, 0, 0);
        drainHeaderLayout->setSpacing(6);
        drainHeaderLayout->addWidget(makeHeader(tr("Drain")));
        drainHeaderLayout->addWidget(makeHeader(tr("FIFO")));
        drainHeaderLayout->addWidget(makeHeader(tr("Type")));
        drainHeaderLayout->addWidget(makeHeader(tr("Depth")));
        drainHeaderLayout->addStretch(1);

        m_outputDrainLayout = new QVBoxLayout;
        m_outputDrainLayout->setContentsMargins(0, 0, 0, 0);
        m_outputDrainLayout->setSpacing(4);

        drainOuterLayout->addWidget(drainHeader);
        drainOuterLayout->addLayout(m_outputDrainLayout);
        m_outputDrainGroup->setVisible(false);

        propertiesLayout->addWidget(m_outputDrainGroup);
        propertiesWidget->setLayout(propertiesLayout);

        propertiesDock->setWidget(propertiesWidget);
        addDockWidget(Qt::RightDockWidgetArea, propertiesDock);

        // Object FIFO panel docked under properties
        auto *objectFifoDock = new QDockWidget(tr("Object Fifo"), this);
        objectFifoDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        auto *objectFifoWidget = new QWidget(objectFifoDock);
        auto *fifoLayout = new QFormLayout(objectFifoWidget);
        fifoLayout->setContentsMargins(8, 8, 8, 8);
        fifoLayout->setSpacing(6);

        m_fifoNameEdit = new QLineEdit(objectFifoWidget);
        m_fifoNameEdit->setPlaceholderText(tr("Optional"));
        m_fifoNameEdit->setEnabled(false);
        connect(m_fifoNameEdit, &QLineEdit::textChanged,
                this, &CoreMainWindow::onNameChanged);

        m_fifoTypeCombo = new QComboBox(objectFifoWidget);
        m_fifoTypeCombo->setEnabled(false);

        m_depthSpin = new QSpinBox(objectFifoWidget);
        m_depthSpin->setMinimum(0);
        m_depthSpin->setMaximum(1000000);
        m_depthSpin->setValue(1);
        m_depthSpin->setAccelerated(true);
        m_depthSpin->setEnabled(false);
        connect(m_depthSpin, qOverload<int>(&QSpinBox::valueChanged),
                this, &CoreMainWindow::onDepthChanged);
        connect(m_fifoTypeCombo, &QComboBox::currentTextChanged,
                this, &CoreMainWindow::onFifoTypeChanged);
        updateObjectFifoValidation();

        fifoLayout->addRow(tr("Name"), m_fifoNameEdit);
        fifoLayout->addRow(tr("Type"), m_fifoTypeCombo);
        fifoLayout->addRow(tr("Depth"), m_depthSpin);

        objectFifoWidget->setLayout(fifoLayout);
        objectFifoDock->setWidget(objectFifoWidget);
        addDockWidget(Qt::RightDockWidgetArea, objectFifoDock);
        splitDockWidget(propertiesDock, objectFifoDock, Qt::Vertical);

        // View menu entries for docks
        QMenu *viewMenu = menuBar()->addMenu(tr("View"));
        const QList<QDockWidget *> docks {
            functionsDock, modulesDock, toolboxDock, symbolDock,
            designDock, propertiesDock, objectFifoDock
        };
        for (auto *dock : docks) {
            if (dock)
                viewMenu->addAction(dock->toggleViewAction());
        }

        // Center: node editor widget with placeholder
        m_editor = new NodeEditorWidget(central);
        m_editor->setEnabled(false);
        m_canvasPlaceholder = new QLabel(tr("Select/create an AIECAD design to open it"), central);
        m_canvasPlaceholder->setAlignment(Qt::AlignCenter);
        m_canvasPlaceholder->setStyleSheet(QStringLiteral("color: rgba(0,0,0,0.35); font-size: 16px;"));
        m_centralStack = new QStackedLayout;
        m_centralStack->addWidget(m_canvasPlaceholder);
        m_centralStack->addWidget(m_editor);
        m_centralStack->setCurrentIndex(0);
        centralLayout->addLayout(m_centralStack, 1);

        setCentralWidget(central);
        resize(1400, 800);

        if (m_editor && m_editor->scene()) {
            connect(m_editor->scene(), &QGraphicsScene::selectionChanged,
                    this, &CoreMainWindow::updatePropertiesPanel);
            connect(m_editor->scene(), &DataFlowGraphicsScene::nodeContextMenu,
                    this, &CoreMainWindow::showNodeContextMenu);
        }

        if (m_editor && m_editor->graphModel()) {
            auto *model = m_editor->graphModel();
            connect(model, &DataFlowGraphModel::connectionCreated,
                    this, &CoreMainWindow::onConnectionCreated);
            connect(model, &DataFlowGraphModel::connectionDeleted,
                    this, &CoreMainWindow::onConnectionDeleted);
            connect(model, &DataFlowGraphModel::modelReset,
                    this, &CoreMainWindow::onGraphModelReset);
            connect(model, &DataFlowGraphModel::nodeCreated, this, [this](NodeId) {
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
            });
            connect(model, &DataFlowGraphModel::nodeDeleted, this, [this](NodeId id) {
                m_nodeGridCoords.erase(id);
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
            });
        }

        if (m_editor) {
            connect(m_editor, &NodeEditorWidget::npuVersionChanged, this, [this](const QString &) {
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
            });
            connect(m_editor, &NodeEditorWidget::generateCodeRequested,
                    this, &CoreMainWindow::onGenerateCode);
            connect(m_editor, &NodeEditorWidget::nodeCreatedWithPosition, this, [this](NodeId) {
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
                updatePropertiesPanel();
            });
            if (m_editor->scene()) {
                connect(m_editor->scene(), &DataFlowGraphicsScene::nodeMoved, this, [this](NodeId) {
                    if (!m_loadingMetadata)
                        saveActiveDesignMetadata();
                });
            }
        }

        updatePropertiesPanel();
        restoreSettings();
    }

    NodeEditorWidget *editor() const { return m_editor; }

private:
    struct EdgeProperties
    {
        QString typeName;
        int depth {1};
        QString name;
    };

    NodeEditorWidget *m_editor { nullptr };
    QLineEdit *m_nameEdit { nullptr };
    QLineEdit *m_typeEdit { nullptr };
    QPlainTextEdit *m_descEdit { nullptr };
    QComboBox *m_fifoTypeCombo { nullptr };
    QSpinBox *m_depthSpin { nullptr };
    QLineEdit *m_fifoNameEdit { nullptr };
    QToolButton *m_inMinus { nullptr };
    QToolButton *m_inPlus { nullptr };
    QToolButton *m_outMinus { nullptr };
    QToolButton *m_outPlus { nullptr };
    QLabel *m_inCountLabel { nullptr };
    QLabel *m_outCountLabel { nullptr };
    QComboBox *m_kernelCombo { nullptr };
    QLabel *m_coordLabel { nullptr };
    QGroupBox *m_entryFillGroup { nullptr };
    QVBoxLayout *m_entryFillLayout { nullptr };
    QList<QWidget *> m_entryFillRows;
    struct EntryFillConfig {
        QString fifoTo;
        QString fifoFrom;
        QString shim;
    };
    std::unordered_map<NodeId, std::vector<EntryFillConfig>> m_entryFillConfig;
    QGroupBox *m_outputDrainGroup { nullptr };
    QVBoxLayout *m_outputDrainLayout { nullptr };
    QList<QWidget *> m_outputDrainRows;
    std::unordered_map<NodeId, std::vector<EntryFillConfig>> m_outputDrainConfig;
    QTableWidget *m_symbolVarTable { nullptr };
    QTableWidget *m_symbolTypeTable { nullptr };
    std::unordered_map<QString, QString> m_tileDefaultTypes;
    std::unordered_map<NodeId, QPoint> m_nodeGridCoords;
    QTreeView *m_designTree { nullptr };
    QStandardItemModel *m_designModel { nullptr };
    int m_designCounter {1};
    QString m_designRootPath;
    QLabel *m_designPathLabel { nullptr };
    QStackedLayout *m_centralStack { nullptr };
    QLabel *m_canvasPlaceholder { nullptr };
    QString m_activeDesignPath;
    bool m_loadingMetadata { false };
    bool m_populateGridFlag { false };
    std::unordered_map<ConnectionId, EdgeProperties> m_connectionProps;
    int m_nextConnectionNameIndex {1};
    std::optional<ConnectionId> m_selectedConnection;
    std::vector<ConnectionId> m_selectedConnections;
    std::optional<NodeId> m_selectedNode;

    void updatePropertiesPanel()
    {
        auto resetFields = [this]() {
            if (m_nameEdit) {
                m_nameEdit->blockSignals(true);
                m_nameEdit->clear();
                m_nameEdit->setPlaceholderText(tr("No selection"));
                m_nameEdit->blockSignals(false);
            }
            if (m_typeEdit) {
                m_typeEdit->blockSignals(true);
                m_typeEdit->clear();
                m_typeEdit->blockSignals(false);
            }
            if (m_coordLabel) {
                m_coordLabel->setText(QStringLiteral("-"));
            }
            if (m_descEdit) {
                m_descEdit->blockSignals(true);
                m_descEdit->clear();
                m_descEdit->setPlaceholderText(tr("Select a node to see properties."));
                m_descEdit->blockSignals(false);
            }
            m_selectedNode.reset();
            refreshPortCounts(nullptr);
            if (m_kernelCombo) {
                QSignalBlocker blocker(m_kernelCombo);
                m_kernelCombo->setCurrentIndex(0);
                m_kernelCombo->setEnabled(false);
            }
            if (m_entryFillGroup) {
                m_entryFillGroup->setVisible(false);
            }
            if (m_entryFillLayout) {
                for (auto *row : m_entryFillRows) {
                    m_entryFillLayout->removeWidget(row);
                    row->deleteLater();
                }
                m_entryFillRows.clear();
            }
            if (m_outputDrainGroup)
                m_outputDrainGroup->setVisible(false);
            if (m_outputDrainLayout) {
                for (auto *row : m_outputDrainRows) {
                    m_outputDrainLayout->removeWidget(row);
                    row->deleteLater();
                }
                m_outputDrainRows.clear();
            }
        };

        auto resetFifoPanel = [this]() {
            m_selectedConnection.reset();
            m_selectedConnections.clear();
            if (m_fifoTypeCombo) {
                QSignalBlocker blocker(m_fifoTypeCombo);
                m_fifoTypeCombo->setCurrentIndex(-1);
                m_fifoTypeCombo->setEnabled(false);
            }
            if (m_depthSpin) {
                QSignalBlocker blocker(m_depthSpin);
                m_depthSpin->setValue(1);
                m_depthSpin->setEnabled(false);
            }
            if (m_fifoNameEdit) {
                QSignalBlocker blocker(m_fifoNameEdit);
                m_fifoNameEdit->clear();
                m_fifoNameEdit->setEnabled(false);
            }
            updateObjectFifoValidation();
        };

        if (!m_editor) {
            resetFields();
            resetFifoPanel();
            return;
        }

        auto *scene = m_editor->scene();
        auto *model = m_editor->graphModel();
        if (!scene || !model) {
            resetFields();
            resetFifoPanel();
            return;
        }

        const auto selected = scene->selectedItems();
        if (selected.isEmpty()) {
            resetFields();
            resetFifoPanel();
            return;
        }

        QVector<ConnectionGraphicsObject *> selectedConnections;
        bool hasNodeSelection = false;
        for (auto *item : selected) {
            if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
                Q_UNUSED(ngo);
                hasNodeSelection = true;
            } else if (auto *cgo = qgraphicsitem_cast<ConnectionGraphicsObject *>(item)) {
                selectedConnections.push_back(cgo);
            }
        }

        const bool onlyConnectionsSelected = !selectedConnections.isEmpty() && !hasNodeSelection;

        // Multiple selection handling
        if (selected.size() > 1 && !onlyConnectionsSelected) {
            resetFields();
            resetFifoPanel();
            if (hasNodeSelection) {
                // Enable port controls for bulk edits; show placeholder counts.
                setPortControlsEnabled(true);
                if (m_inMinus) m_inMinus->setEnabled(true);
                if (m_outMinus) m_outMinus->setEnabled(true);
                if (m_inCountLabel) m_inCountLabel->setText(QStringLiteral("-"));
                if (m_outCountLabel) m_outCountLabel->setText(QStringLiteral("-"));
            } else {
                setPortControlsEnabled(false);
            }
            return;
        }

        auto *first = selected.first();

        if (onlyConnectionsSelected) {
            m_selectedNode.reset();
            m_selectedConnection.reset();
            m_selectedConnections.clear();
            refreshPortCounts(nullptr);
            clearEntryFillRows();
            if (m_entryFillGroup)
                m_entryFillGroup->setVisible(false);
            if (m_kernelCombo) {
                QSignalBlocker blocker(m_kernelCombo);
                m_kernelCombo->setCurrentIndex(0);
                m_kernelCombo->setEnabled(false);
            }

            // Determine common/default properties.
            QString typeValue = defaultSymbolType();
            int depthValue = 1;
            bool firstEdge = true;
            for (auto *cgo : selectedConnections) {
                const ConnectionId cid = cgo->connectionId();
                m_selectedConnections.push_back(cid);
                auto result = m_connectionProps.emplace(cid, EdgeProperties{});
                EdgeProperties &props = result.first->second;
                if (props.typeName.isEmpty())
                    props.typeName = defaultSymbolType();
                if (props.depth <= 0)
                    props.depth = 1;
                if (firstEdge) {
                    typeValue = props.typeName;
                    depthValue = props.depth;
                    firstEdge = false;
                }
            }

            if (m_fifoTypeCombo) {
                QSignalBlocker blocker(m_fifoTypeCombo);
                updateFifoTypeOptions();
                const int idx = std::max(0, m_fifoTypeCombo->findText(typeValue));
                m_fifoTypeCombo->setCurrentIndex(idx);
                m_fifoTypeCombo->setEnabled(true);
            }

            if (m_depthSpin) {
                QSignalBlocker blocker(m_depthSpin);
                m_depthSpin->setValue(depthValue);
                m_depthSpin->setEnabled(true);
            }

        if (m_fifoNameEdit) {
            QSignalBlocker blocker(m_fifoNameEdit);
            m_fifoNameEdit->clear();
            m_fifoNameEdit->setEnabled(false);
            m_fifoNameEdit->setPlaceholderText(selectedConnections.size() > 1
                                               ? tr("Multiple edges selected")
                                               : tr("Edge selected"));
        }

            updateObjectFifoValidation();

            // Clear node-specific properties when edges are selected.
            if (m_nameEdit) {
                QSignalBlocker blocker(m_nameEdit);
                m_nameEdit->clear();
                m_nameEdit->setPlaceholderText(tr("Edge selected (see Object Fifo)"));
            }
            if (m_typeEdit) {
                QSignalBlocker blocker(m_typeEdit);
                m_typeEdit->clear();
                m_typeEdit->setPlaceholderText(tr("Edge selected"));
            }
            if (m_descEdit) {
                QSignalBlocker blocker(m_descEdit);
                m_descEdit->clear();
                m_descEdit->setPlaceholderText(tr("Edge properties shown in Object Fifo"));
            }
            if (m_coordLabel)
                m_coordLabel->setText(QStringLiteral("-"));
            return;
        }

        if (auto *conn = qgraphicsitem_cast<ConnectionGraphicsObject *>(first)) {
            const ConnectionId connectionId = conn->connectionId();
            m_selectedConnection = connectionId;
            m_selectedConnections = {connectionId};
            m_selectedNode.reset();
            refreshPortCounts(nullptr);
            clearEntryFillRows();
            if (m_entryFillGroup)
                m_entryFillGroup->setVisible(false);
            if (m_kernelCombo) {
                QSignalBlocker blocker(m_kernelCombo);
                m_kernelCombo->setCurrentIndex(0);
                m_kernelCombo->setEnabled(false);
            }

            auto result = m_connectionProps.emplace(connectionId, EdgeProperties{});
            EdgeProperties &props = result.first->second;
            if (props.typeName.isEmpty())
                props.typeName = defaultSymbolType();
            if (props.depth <= 0)
                props.depth = 1;
            const QString name = ensureConnectionName(connectionId);

            if (m_fifoTypeCombo) {
                QSignalBlocker blocker(m_fifoTypeCombo);
                updateFifoTypeOptions();
                const int idx = std::max(0, m_fifoTypeCombo->findText(props.typeName));
                m_fifoTypeCombo->setCurrentIndex(idx);
                m_fifoTypeCombo->setEnabled(true);
            }

            if (m_depthSpin) {
                QSignalBlocker blocker(m_depthSpin);
                m_depthSpin->setValue(props.depth);
                m_depthSpin->setEnabled(true);
            }

            if (m_fifoNameEdit) {
                QSignalBlocker blocker(m_fifoNameEdit);
                m_fifoNameEdit->setText(name);
                m_fifoNameEdit->setEnabled(true);
                m_fifoNameEdit->setPlaceholderText(QString());
            }

            updateObjectFifoValidation();

            // Clear node-specific properties when an edge is selected.
            if (m_nameEdit) {
                QSignalBlocker blocker(m_nameEdit);
                m_nameEdit->clear();
                m_nameEdit->setPlaceholderText(tr("Edge selected (see Object Fifo)"));
            }
            if (m_typeEdit) {
                QSignalBlocker blocker(m_typeEdit);
                m_typeEdit->clear();
                m_typeEdit->setPlaceholderText(tr("Edge selected"));
            }
            if (m_descEdit) {
                QSignalBlocker blocker(m_descEdit);
                m_descEdit->clear();
                m_descEdit->setPlaceholderText(tr("Edge properties shown in Object Fifo"));
            }
            if (m_coordLabel)
                m_coordLabel->setText(QStringLiteral("-"));
            return;
        }

        if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(first)) {
            const NodeId nodeId = ngo->nodeId();
            m_selectedNode = nodeId;
            auto *delegate = model->delegateModel<NodeDelegateModel>(nodeId);
            auto *base = dynamic_cast<BaseNodeModel *>(delegate);

            resetFifoPanel();
            m_selectedConnection.reset();
            m_selectedConnections.clear();

            ensureMinimumPorts(base);
            refreshPortCounts(base);
            refreshNodeGeometry(nodeId);

            if (m_kernelCombo) {
                QSignalBlocker blocker(m_kernelCombo);
                const bool isCompute = delegate && delegate->name() == QLatin1String("ComputeNode");
                m_kernelCombo->setEnabled(isCompute);
                m_kernelCombo->setCurrentIndex(0);
            }

            if (m_nameEdit) {
                const QString name = base ? base->customName()
                                          : (delegate ? delegate->caption() : tr("Node"));
                m_nameEdit->setText(name);
            }

            if (m_typeEdit) {
                m_typeEdit->blockSignals(true);
                m_typeEdit->setText(delegate ? delegate->caption() : tr("Node"));
                m_typeEdit->blockSignals(false);
            }
            if (m_coordLabel) {
                auto it = m_nodeGridCoords.find(nodeId);
                if (it != m_nodeGridCoords.end()) {
                    m_coordLabel->setText(tr("x=%1, y=%2").arg(it->second.x()).arg(it->second.y()));
                } else {
                    m_coordLabel->setText(QStringLiteral("-"));
                }
            }

            if (m_descEdit) {
                m_descEdit->blockSignals(true);
                if (base && !base->description().isEmpty()) {
                    m_descEdit->setPlainText(base->description());
                    m_descEdit->setPlaceholderText(tr("Description"));
                } else {
                    m_descEdit->clear();
                    m_descEdit->setPlaceholderText(tr("No description available."));
                }
                m_descEdit->blockSignals(false);
            }
            refreshEntryFillPanel(base, nodeId);
            refreshOutputDrainPanel(base, nodeId);
            return;
        }

        resetFields();
    }

    void updateObjectFifoValidation()
    {
        if (!m_depthSpin)
            return;

        const bool depthInvalid = m_depthSpin && m_depthSpin->value() <= 0;
        if (m_depthSpin) {
            m_depthSpin->setStyleSheet(depthInvalid ? QStringLiteral("QSpinBox { border: 1px solid red; }")
                                                    : QString());
        }
    }

    void clearEntryFillRows()
    {
        if (!m_entryFillLayout)
            return;
        for (auto *row : m_entryFillRows) {
            m_entryFillLayout->removeWidget(row);
            row->deleteLater();
        }
        m_entryFillRows.clear();
    }

    void clearOutputDrainRows()
    {
        if (!m_outputDrainLayout)
            return;
        for (auto *row : m_outputDrainRows) {
            m_outputDrainLayout->removeWidget(row);
            row->deleteLater();
        }
        m_outputDrainRows.clear();
    }

    QStringList availableFifoNames()
    {
        QStringList names;
        for (const auto &cid : allConnections()) {
            names << ensureConnectionName(cid);
        }
        names.removeDuplicates();
        names.sort();
        return names;
    }

    QStringList availableShimNames()
    {
        QStringList names;
        if (!m_editor || !m_editor->graphModel())
            return names;
        for (auto nid : m_editor->graphModel()->allNodeIds()) {
            if (auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(nid)) {
                if (delegate->name() == QLatin1String("ShimTile")) {
                    if (auto *base = dynamic_cast<BaseNodeModel *>(delegate)) {
                        names << base->customName();
                    }
                }
            }
        }
        names.removeDuplicates();
        names.sort();
        return names;
    }

    void refreshEntryFillPanel(BaseNodeModel *base, NodeId nodeId)
    {
        if (!m_entryFillGroup || !m_entryFillLayout) {
            return;
        }

        clearEntryFillRows();

        if (!base || base->name() != QLatin1String("EntryNode")) {
            m_entryFillGroup->setVisible(false);
            return;
        }

        auto connections = allConnections();
        const unsigned int outCount = base->nPorts(PortType::Out);
        auto &configVec = m_entryFillConfig[nodeId];
        if (configVec.size() < outCount)
            configVec.resize(outCount);

        const QStringList fifoNames = availableFifoNames();
        const QStringList shimNames = availableShimNames();

        for (unsigned int i = 0; i < outCount; ++i) {
            const ConnectionId *match = nullptr;
            for (const auto &cid : connections) {
                if (cid.outNodeId == nodeId && cid.outPortIndex == i) {
                    match = &cid;
                    break;
                }
            }

            QString fifoName = tr("Not connected");
            QString fifoType = defaultSymbolType();
            QString depthStr = tr("-");

            if (match) {
                fifoName = ensureConnectionName(*match);
                auto it = m_connectionProps.find(*match);
                if (it != m_connectionProps.end()) {
                    if (!it->second.typeName.isEmpty())
                        fifoType = it->second.typeName;
                    depthStr = QString::number(std::max(1, it->second.depth));
                }
            }

            auto &cfg = configVec[i];
            if (cfg.fifoTo.isEmpty() && !fifoNames.isEmpty())
                cfg.fifoTo = fifoNames.front();
            if (cfg.fifoFrom.isEmpty() && !fifoNames.isEmpty())
                cfg.fifoFrom = fifoNames.front();
            if (cfg.shim.isEmpty() && !shimNames.isEmpty())
                cfg.shim = shimNames.front();

            auto *rowWidget = new QWidget(m_entryFillGroup);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(tr("Fill %1").arg(i), rowWidget));
            rowLayout->addWidget(new QLabel(tr("FIFO:"), rowWidget));

            auto *toCombo = new QComboBox(rowWidget);
            toCombo->addItems(fifoNames);
            int idxTo = toCombo->findText(cfg.fifoTo);
            if (idxTo < 0 && !fifoNames.isEmpty())
                idxTo = 0;
            toCombo->setCurrentIndex(idxTo);

            auto *fromCombo = new QComboBox(rowWidget);
            fromCombo->addItems(fifoNames);
            int idxFrom = fromCombo->findText(cfg.fifoFrom);
            if (idxFrom < 0 && !fifoNames.isEmpty())
                idxFrom = 0;
            fromCombo->setCurrentIndex(idxFrom);

            auto *shimCombo = new QComboBox(rowWidget);
            shimCombo->addItems(shimNames);
            int idxShim = shimCombo->findText(cfg.shim);
            if (idxShim < 0 && !shimNames.isEmpty())
                idxShim = 0;
            shimCombo->setCurrentIndex(idxShim);

            rowLayout->addWidget(new QLabel(tr("To"), rowWidget));
            rowLayout->addWidget(toCombo);
            rowLayout->addWidget(new QLabel(tr("From"), rowWidget));
            rowLayout->addWidget(fromCombo);
            rowLayout->addWidget(new QLabel(tr("Shim"), rowWidget));
            rowLayout->addWidget(shimCombo);
            rowLayout->addSpacing(8);
            rowLayout->addWidget(new QLabel(fifoType, rowWidget));
            rowLayout->addWidget(new QLabel(depthStr, rowWidget));
            rowLayout->addStretch(1);
            m_entryFillLayout->addWidget(rowWidget);
            m_entryFillRows.push_back(rowWidget);

            auto updateCfg = [this, nodeId, i](const QString &to, const QString &from, const QString &shim) {
                auto &vec = m_entryFillConfig[nodeId];
                if (vec.size() <= i)
                    vec.resize(i + 1);
                vec[i].fifoTo = to;
                vec[i].fifoFrom = from;
                vec[i].shim = shim;
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
            };

            QObject::connect(toCombo, &QComboBox::currentTextChanged, this, [fromCombo, shimCombo, updateCfg](const QString &text) {
                updateCfg(text, fromCombo->currentText(), shimCombo->currentText());
            });
            QObject::connect(fromCombo, &QComboBox::currentTextChanged, this, [toCombo, shimCombo, updateCfg](const QString &text) {
                updateCfg(toCombo->currentText(), text, shimCombo->currentText());
            });
            QObject::connect(shimCombo, &QComboBox::currentTextChanged, this, [toCombo, fromCombo, updateCfg](const QString &text) {
                updateCfg(toCombo->currentText(), fromCombo->currentText(), text);
            });
        }

        m_entryFillGroup->setVisible(true);
    }

    void refreshOutputDrainPanel(BaseNodeModel *base, NodeId nodeId)
    {
        if (!m_outputDrainGroup || !m_outputDrainLayout) {
            return;
        }

        clearOutputDrainRows();

        if (!base || base->name() != QLatin1String("OutputNode")) {
            m_outputDrainGroup->setVisible(false);
            return;
        }

        auto connections = allConnections();
        const unsigned int inCount = base->nPorts(PortType::In);
        auto &configVec = m_outputDrainConfig[nodeId];
        if (configVec.size() < inCount)
            configVec.resize(inCount);

        const QStringList fifoNames = availableFifoNames();
        const QStringList shimNames = availableShimNames();

        for (unsigned int i = 0; i < inCount; ++i) {
            const ConnectionId *match = nullptr;
            for (const auto &cid : connections) {
                if (cid.inNodeId == nodeId && cid.inPortIndex == i) {
                    match = &cid;
                    break;
                }
            }

            QString fifoName = tr("Not connected");
            QString fifoType = defaultSymbolType();
            QString depthStr = tr("-");

            if (match) {
                fifoName = ensureConnectionName(*match);
                auto it = m_connectionProps.find(*match);
                if (it != m_connectionProps.end()) {
                    if (!it->second.typeName.isEmpty())
                        fifoType = it->second.typeName;
                    depthStr = QString::number(std::max(1, it->second.depth));
                }
            }

            auto &cfg = configVec[i];
            if (cfg.fifoTo.isEmpty() && !fifoNames.isEmpty())
                cfg.fifoTo = fifoNames.front();
            if (cfg.fifoFrom.isEmpty() && !fifoNames.isEmpty())
                cfg.fifoFrom = fifoNames.front();
            if (cfg.shim.isEmpty() && !shimNames.isEmpty())
                cfg.shim = shimNames.front();

            auto *rowWidget = new QWidget(m_outputDrainGroup);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(new QLabel(tr("Drain %1").arg(i), rowWidget));
            rowLayout->addWidget(new QLabel(tr("FIFO:"), rowWidget));

            auto *toCombo = new QComboBox(rowWidget);
            toCombo->addItems(fifoNames);
            int idxTo = toCombo->findText(cfg.fifoTo);
            if (idxTo < 0 && !fifoNames.isEmpty())
                idxTo = 0;
            toCombo->setCurrentIndex(idxTo);

            auto *fromCombo = new QComboBox(rowWidget);
            fromCombo->addItems(fifoNames);
            int idxFrom = fromCombo->findText(cfg.fifoFrom);
            if (idxFrom < 0 && !fifoNames.isEmpty())
                idxFrom = 0;
            fromCombo->setCurrentIndex(idxFrom);

            auto *shimCombo = new QComboBox(rowWidget);
            shimCombo->addItems(shimNames);
            int idxShim = shimCombo->findText(cfg.shim);
            if (idxShim < 0 && !shimNames.isEmpty())
                idxShim = 0;
            shimCombo->setCurrentIndex(idxShim);

            rowLayout->addWidget(new QLabel(tr("To"), rowWidget));
            rowLayout->addWidget(toCombo);
            rowLayout->addWidget(new QLabel(tr("From"), rowWidget));
            rowLayout->addWidget(fromCombo);
            rowLayout->addWidget(new QLabel(tr("Shim"), rowWidget));
            rowLayout->addWidget(shimCombo);
            rowLayout->addSpacing(8);
            rowLayout->addWidget(new QLabel(fifoType, rowWidget));
            rowLayout->addWidget(new QLabel(depthStr, rowWidget));
            rowLayout->addStretch(1);
            m_outputDrainLayout->addWidget(rowWidget);
            m_outputDrainRows.push_back(rowWidget);

            auto updateCfg = [this, nodeId, i](const QString &to, const QString &from, const QString &shim) {
                auto &vec = m_outputDrainConfig[nodeId];
                if (vec.size() <= i)
                    vec.resize(i + 1);
                vec[i].fifoTo = to;
                vec[i].fifoFrom = from;
                vec[i].shim = shim;
                if (!m_loadingMetadata)
                    saveActiveDesignMetadata();
            };

            QObject::connect(toCombo, &QComboBox::currentTextChanged, this, [fromCombo, shimCombo, updateCfg](const QString &text) {
                updateCfg(text, fromCombo->currentText(), shimCombo->currentText());
            });
            QObject::connect(fromCombo, &QComboBox::currentTextChanged, this, [toCombo, shimCombo, updateCfg](const QString &text) {
                updateCfg(toCombo->currentText(), text, shimCombo->currentText());
            });
            QObject::connect(shimCombo, &QComboBox::currentTextChanged, this, [toCombo, fromCombo, updateCfg](const QString &text) {
                updateCfg(toCombo->currentText(), fromCombo->currentText(), text);
            });
        }

        m_outputDrainGroup->setVisible(true);
    }


    void setPortControlsEnabled(bool enabled)
    {
        if (m_inMinus) m_inMinus->setEnabled(enabled);
        if (m_inPlus) m_inPlus->setEnabled(enabled);
        if (m_outMinus) m_outMinus->setEnabled(enabled);
        if (m_outPlus) m_outPlus->setEnabled(enabled);
        if (m_inCountLabel) m_inCountLabel->setEnabled(enabled);
        if (m_outCountLabel) m_outCountLabel->setEnabled(enabled);
    }

    BaseNodeModel *currentBaseNode() const
    {
        if (!m_editor || !m_editor->graphModel() || !m_selectedNode)
            return nullptr;

        return dynamic_cast<BaseNodeModel *>(
            m_editor->graphModel()->delegateModel<NodeDelegateModel>(*m_selectedNode));
    }

    QList<BaseNodeModel *> currentBaseNodes() const
    {
        QList<BaseNodeModel *> nodes;
        if (!m_editor || !m_editor->scene() || !m_editor->graphModel())
            return nodes;

        for (auto *item : m_editor->scene()->selectedItems()) {
            if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
                if (auto *model = m_editor->graphModel()->delegateModel<NodeDelegateModel>(ngo->nodeId())) {
                    if (auto *base = dynamic_cast<BaseNodeModel *>(model)) {
                        nodes.push_back(base);
                    }
                }
            }
        }
        return nodes;
    }

    void refreshPortCounts(BaseNodeModel *base)
    {
        if (!base) {
            if (m_inCountLabel) m_inCountLabel->setText(QStringLiteral("-"));
            if (m_outCountLabel) m_outCountLabel->setText(QStringLiteral("-"));
            setPortControlsEnabled(false);
            return;
        }

        unsigned int inCount = base->nPorts(PortType::In);
        unsigned int outCount = base->nPorts(PortType::Out);

        if (m_inCountLabel) m_inCountLabel->setText(QString::number(inCount));
        if (m_outCountLabel) m_outCountLabel->setText(QString::number(outCount));

        const auto pc = constraintsFor(base);

        setPortControlsEnabled(true);
        if (m_inMinus) m_inMinus->setEnabled(pc.allowIn && inCount > pc.minIn);
        if (m_inPlus) m_inPlus->setEnabled(pc.allowIn);
        if (m_outMinus) m_outMinus->setEnabled(pc.allowOut && outCount > pc.minOut);
        if (m_outPlus) m_outPlus->setEnabled(pc.allowOut);
    }

    struct PortConstraints
    {
        unsigned int minIn {0};
        unsigned int minOut {0};
        bool allowIn {true};
        bool allowOut {true};
    };

    PortConstraints constraintsFor(BaseNodeModel *base) const
    {
        PortConstraints pc{1, 1, true, true};
        if (!base)
            return pc;

        const QString name = base->name();
        if (name == QLatin1String("EntryNode")) {
            pc.minIn = 0;
            pc.minOut = 1;
            pc.allowIn = false;
        } else if (name == QLatin1String("OutputNode")) {
            pc.minIn = 1;
            pc.minOut = 0;
            pc.allowOut = false;
        }
        return pc;
    }

    void ensureMinimumPorts(BaseNodeModel *base)
    {
        if (!base)
            return;

        const auto pc = constraintsFor(base);

        // Remove disallowed inputs/outputs
        if (!pc.allowIn) {
            while (base->nPorts(PortType::In) > 0)
                base->removeInPort();
        }
        if (!pc.allowOut) {
            while (base->nPorts(PortType::Out) > 0)
                base->removeOutPort();
        }

        while (base->nPorts(PortType::In) < pc.minIn) {
            base->addInPort();
        }
        while (base->nPorts(PortType::Out) < pc.minOut) {
            base->addOutPort();
        }
    }

    void refreshNodeGeometry(NodeId nodeId)
    {
        if (!m_editor || !m_editor->scene())
            return;

        auto *scene = m_editor->scene();
        if (auto *ngo = scene->nodeGraphicsObject(nodeId)) {
            scene->nodeGeometry().recomputeSize(nodeId);
            ngo->updateQWidgetEmbedPos();
            ngo->update();
            ngo->moveConnections();
        }
    }

    void refreshSelectedNodeGeometries()
    {
        if (!m_editor || !m_editor->scene())
            return;

        const auto items = m_editor->scene()->selectedItems();
        for (auto *item : items) {
            if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item)) {
                refreshNodeGeometry(ngo->nodeId());
            }
        }
    }

    void addSymbolVariableRow()
    {
        if (!m_symbolVarTable)
            return;

        const int row = m_symbolVarTable->rowCount();
        m_symbolVarTable->insertRow(row);
        m_symbolVarTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("var%1").arg(row + 1)));
        m_symbolVarTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("0")));
        m_symbolVarTable->selectRow(row);
        updateTypeDimsValidation();
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

    void removeSymbolVariableRow()
    {
        if (!m_symbolVarTable)
            return;

        int row = m_symbolVarTable->currentRow();
        if (row < 0)
            row = m_symbolVarTable->rowCount() - 1;

        if (row >= 0)
            m_symbolVarTable->removeRow(row);

        if (m_symbolVarTable->rowCount() > 0)
            m_symbolVarTable->selectRow(std::min(row, m_symbolVarTable->rowCount() - 1));

        updateTypeDimsValidation();
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

    QStringList symbolVariableNames() const
    {
        QStringList names;
        if (!m_symbolVarTable)
            return names;

        for (int row = 0; row < m_symbolVarTable->rowCount(); ++row) {
            if (auto *item = m_symbolVarTable->item(row, 0)) {
                const QString name = item->text().trimmed();
                if (!name.isEmpty())
                    names << name;
            }
        }
        return names;
    }

    QStringList symbolVariableCompletions() const
    {
        QStringList comps;
        for (const auto &name : symbolVariableNames()) {
            comps << QStringLiteral("$%1").arg(name);
        }
        return comps;
    }

    void clearSymbolDefinitions()
    {
        QSignalBlocker b1(m_symbolVarTable);
        if (m_symbolVarTable)
            m_symbolVarTable->setRowCount(0);

        QSignalBlocker b2(m_symbolTypeTable);
        if (m_symbolTypeTable)
            m_symbolTypeTable->setRowCount(0);

        updateFifoTypeOptions();
    }

    QStringList symbolTypeNames() const
    {
        QStringList names;
        if (!m_symbolTypeTable)
            return names;

        for (int row = 0; row < m_symbolTypeTable->rowCount(); ++row) {
            if (auto *item = m_symbolTypeTable->item(row, 0)) {
                const QString name = item->text().trimmed();
                if (!name.isEmpty())
                    names << name;
            }
        }
        return names;
    }

    QString defaultSymbolType() const
    {
        const auto names = symbolTypeNames();
        return names.isEmpty() ? QString() : names.front();
    }

    QString metadataFilePath(const QString &designDir) const
    {
        const QString base = QFileInfo(designDir).fileName();
        return QDir(designDir).filePath(base);
    }

    QString nextDefaultConnectionName()
    {
        return QStringLiteral("of%1").arg(m_nextConnectionNameIndex++);
    }

    QString ensureConnectionName(const ConnectionId &cid)
    {
        auto it = m_connectionProps.find(cid);
        if (it == m_connectionProps.end())
            return {};
        if (it->second.name.isEmpty())
            it->second.name = nextDefaultConnectionName();
        return it->second.name;
    }

    void clearGraph()
    {
        if (!m_editor || !m_editor->graphModel())
            return;
        auto ids = m_editor->graphModel()->allNodeIds();
        for (auto id : ids)
            m_editor->graphModel()->deleteNode(id);
        m_connectionProps.clear();
        m_nextConnectionNameIndex = 1;
        m_selectedConnection.reset();
        m_selectedConnections.clear();
        m_nodeGridCoords.clear();
    }

    std::unordered_set<ConnectionId> allConnections() const
    {
        std::unordered_set<ConnectionId> conns;
        if (!m_editor || !m_editor->graphModel())
            return conns;

        for (auto nid : m_editor->graphModel()->allNodeIds()) {
            auto setForNode = m_editor->graphModel()->allConnectionIds(nid);
            conns.insert(setForNode.begin(), setForNode.end());
        }
        return conns;
    }

    void populateTileGridForDevice(const QString &device)
    {
        if (!m_editor || !m_editor->graphModel())
            return;

        auto *graph = m_editor->graphModel();
        if (!graph->allNodeIds().empty())
            return; // avoid re-populating an existing canvas

        const QString lower = device.toLower();
        const bool isV1 = lower.contains(QStringLiteral("npu1")) || lower.contains(QStringLiteral("v1"));
        const QString resolvedDevice = isV1 ? QStringLiteral("npu1") : QStringLiteral("npu2");

        const int columns = isV1 ? 4 : 8;
        const int rows = 6;
        const double xSpacing = 160.0;
        const double ySpacing = 140.0;
        const QPointF origin(0.0, 0.0);

        const bool previousLoading = m_loadingMetadata;
        m_loadingMetadata = true; // suppress per-node autosaves
        m_editor->setNpuVersion(resolvedDevice);

        auto addTileAt = [&](const QString &typeId, int col, int row) {
            const QPointF pos(origin.x() + col * xSpacing,
                              origin.y() + (rows - 1 - row) * ySpacing);
            NodeId id = graph->addNode(typeId);
            if (id == InvalidNodeId)
                return;
            graph->setNodeData(id, NodeRole::Position, pos);
            refreshNodeGeometry(id);
            m_nodeGridCoords[id] = QPoint(col, row);
            if (auto *delegate = graph->delegateModel<NodeDelegateModel>(id)) {
                if (auto *base = dynamic_cast<BaseNodeModel *>(delegate)) {
                    QString baseName = base->name();
                    if (baseName.endsWith(QStringLiteral("Node")))
                        baseName.chop(QStringLiteral("Node").size());
                    if (baseName.endsWith(QStringLiteral("Tile")))
                        baseName.chop(QStringLiteral("Tile").size());
                    base->setCustomName(QStringLiteral("%1_%2x%3y").arg(baseName).arg(col).arg(row));
                }
            }
        };

        for (int col = 0; col < columns; ++col) {
            addTileAt(QStringLiteral("ShimTile"), col, 0);
            addTileAt(QStringLiteral("MemoryTile"), col, 1);
            for (int row = 2; row < rows; ++row)
                addTileAt(QStringLiteral("ComputeNode"), col, row);
        }

        m_loadingMetadata = previousLoading;
        if (!previousLoading)
            saveActiveDesignMetadata();
        if (m_editor->scene())
            m_editor->scene()->update();
    }

    void setCanvasActive(const QString &designPath)
    {
        m_activeDesignPath = designPath;
        if (m_centralStack)
            m_centralStack->setCurrentIndex(1);
        if (m_editor)
            m_editor->setEnabled(true);
    }

    void setCanvasInactive(const QString &message = QString())
    {
        m_activeDesignPath.clear();
        m_populateGridFlag = false;
        if (m_centralStack)
            m_centralStack->setCurrentIndex(0);
        if (m_editor)
            m_editor->setEnabled(false);
        if (m_canvasPlaceholder && !message.isEmpty())
            m_canvasPlaceholder->setText(message);
        m_nodeGridCoords.clear();
    }

    bool designNameExists(const QString &name) const
    {
        if (!m_designModel)
            return false;
        for (int row = 0; row < m_designModel->rowCount(); ++row) {
            if (auto *item = m_designModel->item(row, 0)) {
                if (item->text().compare(name, Qt::CaseInsensitive) == 0)
                    return true;
            }
        }
        return false;
    }

    void appendDesign(const QString &name, const QString &npuVersion)
    {
        if (!m_designModel)
            return;

        auto *rootItem = new QStandardItem(name);
        rootItem->setEditable(false);
        rootItem->setData(npuVersion, Qt::UserRole);

        // Populate known outputs for this design if they exist beside the metadata.
        const QString designDir = QDir(m_designRootPath).filePath(name);
        const QString baseName = QFileInfo(name).completeBaseName();
        QStringList children;
        children << QStringLiteral("generated_%1.py").arg(baseName)
                 << QStringLiteral("%1_gui.xml").arg(baseName)
                 << QStringLiteral("%1_complete.xml").arg(baseName)
                 << QStringLiteral("%1.graphml").arg(baseName);
        for (const auto &child : children) {
            const QString path = QDir(designDir).filePath(child);
            if (QFileInfo::exists(path)) {
                auto *childItem = new QStandardItem(child);
                childItem->setEditable(false);
                rootItem->appendRow(childItem);
            }
        }

        m_designModel->appendRow(rootItem);
        if (m_designTree) {
            QModelIndex idx = m_designModel->indexFromItem(rootItem);
            m_designTree->expand(idx);
            m_designTree->setCurrentIndex(idx);
        }
    }

    void setDesignRootPath(const QString &dir)
    {
        m_designRootPath = dir;
        if (m_designPathLabel) {
            const QString display = QDir::toNativeSeparators(dir.isEmpty() ? tr("No folder selected") : dir);
            m_designPathLabel->setText(display);
            m_designPathLabel->setToolTip(display);
        }

        QSettings settings(QStringLiteral("AIECAD"), QStringLiteral("CorePlugin"));
        settings.setValue(QStringLiteral("designRootPath"), dir);
    }

    void updateFifoTypeOptions()
    {
        if (!m_fifoTypeCombo)
            return;

        const QString previous = m_fifoTypeCombo->currentText();
        const auto types = symbolTypeNames();

        {
            QSignalBlocker blocker(m_fifoTypeCombo);
            m_fifoTypeCombo->clear();
            m_fifoTypeCombo->addItems(types);
        }

        int idx = m_fifoTypeCombo->findText(previous);
        if (idx < 0 && !types.isEmpty())
            idx = 0;

        {
            QSignalBlocker blocker(m_fifoTypeCombo);
            m_fifoTypeCombo->setCurrentIndex(idx);
        }

        const bool enable = !m_selectedConnections.empty() && !types.isEmpty();
        m_fifoTypeCombo->setEnabled(enable);
    }

    bool saveActiveDesignMetadata()
    {
        if (m_loadingMetadata || m_activeDesignPath.isEmpty() || !m_editor || !m_editor->graphModel())
            return false;

        QJsonObject root;
        root["device"] = m_editor->npuVersionString();
        root["populate_grid"] = m_populateGridFlag;

        QJsonArray vars;
        if (m_symbolVarTable) {
            for (int row = 0; row < m_symbolVarTable->rowCount(); ++row) {
                const QString name = m_symbolVarTable->item(row, 0) ? m_symbolVarTable->item(row, 0)->text().trimmed() : QString();
                const QString value = m_symbolVarTable->item(row, 1) ? m_symbolVarTable->item(row, 1)->text().trimmed() : QString();
                if (name.isEmpty())
                    continue;
                QJsonObject v;
                v["name"] = name;
                v["value"] = value;
                vars.append(v);
            }
        }
        root["variables"] = vars;

        QJsonArray types;
        if (m_symbolTypeTable) {
            for (int row = 0; row < m_symbolTypeTable->rowCount(); ++row) {
                const QString name = m_symbolTypeTable->item(row, 0) ? m_symbolTypeTable->item(row, 0)->text().trimmed() : QString();
                const QString dims = m_symbolTypeTable->item(row, 1) ? m_symbolTypeTable->item(row, 1)->text().trimmed() : QString();
                QString dtype;
                if (auto *combo = qobject_cast<QComboBox *>(m_symbolTypeTable->cellWidget(row, 2)))
                    dtype = combo->currentText();
                else if (auto *item = m_symbolTypeTable->item(row, 2))
                    dtype = item->data(Qt::UserRole).toString();
                if (name.isEmpty())
                    continue;
                QJsonObject t;
                t["name"] = name;
                t["dimensions"] = dims;
                t["type"] = dtype;
                types.append(t);
            }
        }
        root["types"] = types;

        QJsonArray nodesArray;
        for (auto nid : m_editor->graphModel()->allNodeIds()) {
            if (auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(nid)) {
                QJsonObject nodeJson;
                nodeJson["id"] = static_cast<qint64>(nid);
                nodeJson["internal-data"] = delegate->save();
                nodeJson["model"] = delegate->name();
                if (auto *base = dynamic_cast<BaseNodeModel *>(delegate)) {
                    nodeJson["custom_name"] = base->customName();
                    nodeJson["in_ports"] = static_cast<int>(base->nPorts(PortType::In));
                    nodeJson["out_ports"] = static_cast<int>(base->nPorts(PortType::Out));
                    auto coordIt = m_nodeGridCoords.find(nid);
                    if (coordIt != m_nodeGridCoords.end()) {
                        QJsonObject coord;
                        coord["x"] = coordIt->second.x();
                        coord["y"] = coordIt->second.y();
                        nodeJson["grid_coord"] = coord;
                    }
                }
                QPointF pos = m_editor->graphModel()->nodeData(nid, NodeRole::Position).value<QPointF>();
                QJsonObject posJson;
                posJson["x"] = pos.x();
                posJson["y"] = pos.y();
                nodeJson["position"] = posJson;
                nodesArray.append(nodeJson);
            }
        }
        root["nodes"] = nodesArray;

        QJsonArray connectionsArray;
        QJsonArray fifosArray;
        for (const auto &cid : allConnections()) {
            QJsonObject connJson = toJson(cid);
            auto it = m_connectionProps.find(cid);
            if (it != m_connectionProps.end()) {
                const QString type = it->second.typeName.isEmpty() ? defaultSymbolType() : it->second.typeName;
                const int depth = it->second.depth <= 0 ? 1 : it->second.depth;
                const QString name = ensureConnectionName(cid);

                QJsonObject props;
                props["type"] = type;
                props["depth"] = depth;
                props["name"] = name;
                connJson["props"] = props;

                QJsonObject fifo;
                fifo["out_node"] = static_cast<qint64>(cid.outNodeId);
                fifo["out_port"] = static_cast<int>(cid.outPortIndex);
                fifo["in_node"] = static_cast<qint64>(cid.inNodeId);
                fifo["in_port"] = static_cast<int>(cid.inPortIndex);
                fifo["type"] = type;
                fifo["depth"] = depth;
                fifo["name"] = name;
                fifosArray.append(fifo);
            }
            connectionsArray.append(connJson);
        }
        root["connections"] = connectionsArray;
        root["object_fifos"] = fifosArray;

        const QString metaPath = metadataFilePath(m_activeDesignPath);
        QFile file(metaPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    void loadDesignMetadata(const QString &metaPath)
    {
        QFile file(metaPath);
        if (!file.exists()) {
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }
        if (!file.open(QIODevice::ReadOnly))
            return;

        const auto doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isObject())
            return;

        m_loadingMetadata = true;

        clearSymbolDefinitions();
        clearGraph();
        m_populateGridFlag = false;

        const QJsonObject root = doc.object();
        const QString device = root.value(QStringLiteral("device")).toString(QStringLiteral("npu2"));
        const bool populateGrid = root.value(QStringLiteral("populate_grid")).toBool(false);
        m_populateGridFlag = populateGrid;
        if (m_editor)
            m_editor->setNpuVersion(device);

        // Variables
        if (m_symbolVarTable) {
            const auto vars = root.value(QStringLiteral("variables")).toArray();
            m_symbolVarTable->setRowCount(vars.size());
            for (int i = 0; i < vars.size(); ++i) {
                const auto obj = vars[i].toObject();
                m_symbolVarTable->setItem(i, 0, new QTableWidgetItem(obj.value(QStringLiteral("name")).toString()));
                m_symbolVarTable->setItem(i, 1, new QTableWidgetItem(obj.value(QStringLiteral("value")).toString()));
            }
        }

        // Types
        if (m_symbolTypeTable) {
            const auto types = root.value(QStringLiteral("types")).toArray();
            m_symbolTypeTable->setRowCount(types.size());
            for (int i = 0; i < types.size(); ++i) {
                const auto obj = types[i].toObject();
                m_symbolTypeTable->setItem(i, 0, new QTableWidgetItem(obj.value(QStringLiteral("name")).toString()));
                m_symbolTypeTable->setItem(i, 1, new QTableWidgetItem(obj.value(QStringLiteral("dimensions")).toString()));
                attachTypeCombo(i, obj.value(QStringLiteral("type")).toString(QStringLiteral("int32")));
            }
            updateTypeDimsValidation();
            refreshTypeDefaultTooltips();
            updateFifoTypeOptions();
        }

        // Nodes
        std::unordered_map<NodeId, NodeId> idMap;
        if (m_editor && m_editor->graphModel()) {
            const auto nodesArray = root.value(QStringLiteral("nodes")).toArray();
            for (const auto &nodeVal : nodesArray) {
                const auto nodeObj = nodeVal.toObject();
                const NodeId oldId = static_cast<NodeId>(nodeObj.value(QStringLiteral("id")).toInt());
                const QString modelName = nodeObj.value(QStringLiteral("model")).toString();
                NodeId newId = m_editor->graphModel()->addNode(modelName);

                const auto posObj = nodeObj.value(QStringLiteral("position")).toObject();
                QPointF pos(posObj.value(QStringLiteral("x")).toDouble(),
                            posObj.value(QStringLiteral("y")).toDouble());
                m_editor->graphModel()->setNodeData(newId, NodeRole::Position, pos);

                if (auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(newId)) {
                    delegate->load(nodeObj.value(QStringLiteral("internal-data")).toObject());
                    if (auto *base = dynamic_cast<BaseNodeModel *>(delegate)) {
                        const int inPorts = nodeObj.value(QStringLiteral("in_ports")).toInt(base->nPorts(PortType::In));
                        const int outPorts = nodeObj.value(QStringLiteral("out_ports")).toInt(base->nPorts(PortType::Out));
                        const QString customName = nodeObj.value(QStringLiteral("custom_name")).toString();
                        base->setCustomName(customName);
                        while (base->nPorts(PortType::In) < static_cast<unsigned int>(inPorts))
                            base->addInPort();
                        while (base->nPorts(PortType::Out) < static_cast<unsigned int>(outPorts))
                            base->addOutPort();
                        while (base->nPorts(PortType::In) > static_cast<unsigned int>(inPorts))
                            base->removeInPort();
                        while (base->nPorts(PortType::Out) > static_cast<unsigned int>(outPorts))
                            base->removeOutPort();
                        refreshNodeGeometry(newId);
                    }
                }

                const auto gridObj = nodeObj.value(QStringLiteral("grid_coord")).toObject();
                if (gridObj.contains(QStringLiteral("x")) && gridObj.contains(QStringLiteral("y"))) {
                    const int gx = gridObj.value(QStringLiteral("x")).toInt();
                    const int gy = gridObj.value(QStringLiteral("y")).toInt();
                    m_nodeGridCoords[newId] = QPoint(gx, gy);
                }

                idMap[oldId] = newId;
            }

            const auto connArray = root.value(QStringLiteral("connections")).toArray();
            for (const auto &cVal : connArray) {
                const auto cObj = cVal.toObject();
                ConnectionId oldCid = fromJson(cObj);
                ConnectionId newCid {
                    idMap.count(oldCid.outNodeId) ? idMap[oldCid.outNodeId] : oldCid.outNodeId,
                    oldCid.outPortIndex,
                    idMap.count(oldCid.inNodeId) ? idMap[oldCid.inNodeId] : oldCid.inNodeId,
                    oldCid.inPortIndex
                };
                m_editor->graphModel()->addConnection(newCid);
                EdgeProperties props;
                const auto propsObj = cObj.value(QStringLiteral("props")).toObject();
                props.typeName = propsObj.value(QStringLiteral("type")).toString(defaultSymbolType());
                props.depth = propsObj.value(QStringLiteral("depth")).toInt(1);
                props.name = propsObj.value(QStringLiteral("name")).toString();
                if (props.name.isEmpty())
                    props.name = nextDefaultConnectionName();
                m_connectionProps[newCid] = props;
            }

            const auto fifos = root.value(QStringLiteral("object_fifos")).toArray();
            for (const auto &fVal : fifos) {
                const auto fObj = fVal.toObject();
                const NodeId oldOut = static_cast<NodeId>(fObj.value(QStringLiteral("out_node")).toInt());
                const NodeId oldIn  = static_cast<NodeId>(fObj.value(QStringLiteral("in_node")).toInt());
                const PortIndex outPort = static_cast<PortIndex>(fObj.value(QStringLiteral("out_port")).toInt());
                const PortIndex inPort  = static_cast<PortIndex>(fObj.value(QStringLiteral("in_port")).toInt());

                ConnectionId cid {
                    idMap.count(oldOut) ? idMap[oldOut] : oldOut,
                    outPort,
                    idMap.count(oldIn) ? idMap[oldIn] : oldIn,
                    inPort
                };

                if (!m_editor->graphModel()->connectionExists(cid))
                    m_editor->graphModel()->addConnection(cid);

                EdgeProperties &props = m_connectionProps[cid];
                props.typeName = fObj.value(QStringLiteral("type")).toString(props.typeName.isEmpty() ? defaultSymbolType() : props.typeName);
                props.depth = fObj.value(QStringLiteral("depth")).toInt(props.depth <= 0 ? 1 : props.depth);
                const QString fifoName = fObj.value(QStringLiteral("name")).toString(props.name);
                if (!fifoName.isEmpty())
                    props.name = fifoName;
                if (props.name.isEmpty())
                    props.name = nextDefaultConnectionName();
            }
        }

        m_loadingMetadata = false;
        m_nextConnectionNameIndex = static_cast<int>(m_connectionProps.size()) + 1;
        setCanvasActive(QFileInfo(metaPath).absolutePath());
        if (populateGrid && m_editor && m_editor->graphModel() &&
            m_editor->graphModel()->allNodeIds().empty()) {
            populateTileGridForDevice(device);
        }
        updatePropertiesPanel();
    }

    void loadDesignFolder(const QString &dir)
    {
        QDir rootDir(dir);
        if (!rootDir.exists())
            return;

        if (m_designModel) {
            m_designModel->clear();
            m_designModel->setHorizontalHeaderLabels({ tr("Design Explorer") });
        }

        setDesignRootPath(dir);
        setCanvasInactive(tr("Select/create an AIECAD design to open it"));

        const QFileInfoList entries = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &info : entries) {
            if (!info.fileName().endsWith(QLatin1String(".aiecad")))
                continue;

            const QString designName = info.fileName();
            if (designNameExists(designName))
                continue;

            appendDesign(designName, QString());
        }

        if (m_designTree) {
            m_designTree->expandAll();
            if (m_designModel && m_designModel->rowCount() > 0)
                m_designTree->setCurrentIndex(m_designModel->index(0, 0));
        }
    }

    std::optional<QString> normalizeDimensionsWithVars(const QString &text,
                                                       const QStringList &vars) const
    {
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty())
            return QString();

        const auto parts = trimmed.split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (parts.size() > 3)
            return std::nullopt;

        QStringList normalized;
        normalized.reserve(parts.size());

        for (const auto &part : parts) {
            const QString token = part.trimmed();
            if (token.isEmpty())
                return std::nullopt;

            bool ok = false;
            const int value = token.toInt(&ok);
            if (ok) {
                if (value < 0)
                    return std::nullopt;
                normalized << QString::number(value);
                continue;
            }

            if (token.startsWith(QLatin1Char('$'))) {
                const QString name = token.mid(1);
                if (name.isEmpty())
                    return std::nullopt;
                if (!vars.contains(name))
                    return std::nullopt;
                normalized << QStringLiteral("$%1").arg(name);
                continue;
            }

            return std::nullopt;
        }

        return normalized.join(QStringLiteral(", "));
    }

    void updateTypeDimsValidation()
    {
        if (!m_symbolTypeTable)
            return;

        const auto vars = symbolVariableNames();
        for (int row = 0; row < m_symbolTypeTable->rowCount(); ++row) {
            auto *item = m_symbolTypeTable->item(row, 1);
            if (!item)
                continue;

            const auto normalized = normalizeDimensionsWithVars(item->text(), vars);
            const bool invalid = !normalized.has_value();
            item->setBackground(invalid ? QBrush(QColor(QStringLiteral("#ffe5e5")))
                                        : QBrush());
            if (invalid && !item->text().trimmed().isEmpty()) {
                item->setToolTip(tr("Enter up to 3 non-negative integers or $variable references."));
            } else {
                item->setToolTip(QString());
            }
        }
    }

    void refreshTypeDefaultTooltips()
    {
        if (!m_symbolTypeTable)
            return;

        // Drop defaults that no longer have a matching type name.
        QStringList existingNames;
        for (int row = 0; row < m_symbolTypeTable->rowCount(); ++row) {
            if (auto *item = m_symbolTypeTable->item(row, 0))
                existingNames << item->text().trimmed();
        }
        for (auto it = m_tileDefaultTypes.begin(); it != m_tileDefaultTypes.end(); ) {
            if (!existingNames.contains(it->second))
                it = m_tileDefaultTypes.erase(it);
            else
                ++it;
        }

        for (int row = 0; row < m_symbolTypeTable->rowCount(); ++row) {
            auto *item = m_symbolTypeTable->item(row, 0);
            if (!item)
                continue;

            const QString name = item->text().trimmed();
            QStringList roles;
            for (const auto &entry : m_tileDefaultTypes) {
                if (entry.second == name)
                    roles << entry.first;
            }

            if (!roles.isEmpty()) {
                item->setToolTip(tr("Default for: %1").arg(roles.join(QStringLiteral(", "))));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            } else {
                item->setToolTip(QString());
                QFont f = item->font();
                f.setBold(false);
                item->setFont(f);
            }
        }
    }

    void attachTypeCombo(int row, const QString &value)
    {
        if (!m_symbolTypeTable || row < 0 || row >= m_symbolTypeTable->rowCount())
            return;

        auto *combo = new QComboBox(m_symbolTypeTable);
        combo->addItems({ QStringLiteral("int8"),
                          QStringLiteral("int16"),
                          QStringLiteral("int32") });
        const int idx = std::max(0, combo->findText(value));
        combo->setCurrentIndex(idx);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_symbolTypeTable->setCellWidget(row, 2, combo);

        if (!m_symbolTypeTable->item(row, 2)) {
            auto *item = new QTableWidgetItem;
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setText(QString());
            m_symbolTypeTable->setItem(row, 2, item);
        } else {
            m_symbolTypeTable->item(row, 2)->setText(QString());
        }

        connect(combo, &QComboBox::currentTextChanged, this, [this, row](const QString &text) {
            if (m_symbolTypeTable && m_symbolTypeTable->item(row, 2))
                m_symbolTypeTable->item(row, 2)->setData(Qt::UserRole, text);
            refreshTypeDefaultTooltips();
            if (!m_loadingMetadata)
                saveActiveDesignMetadata();
        });
    }

    void showTypeContextMenu(const QPoint &pos)
    {
        if (!m_symbolTypeTable)
            return;

        const QModelIndex idx = m_symbolTypeTable->indexAt(pos);
        if (!idx.isValid())
            return;

        const int row = idx.row();
        auto *nameItem = m_symbolTypeTable->item(row, 0);
        if (!nameItem)
            return;
        const QString typeName = nameItem->text().trimmed();

        QMenu menu(this);
        auto *setDefaultMenu = menu.addMenu(tr("Set Default To..."));
        auto *tileMenu = setDefaultMenu->addMenu(tr("Tile"));
        auto *allMenu = setDefaultMenu->addMenu(tr("All"));

        auto addDefaultAction = [this, typeName](QMenu *parent, const QString &tileLabel) {
            QAction *act = parent->addAction(tileLabel);
            connect(act, &QAction::triggered, this, [this, typeName, tileLabel]() {
                m_tileDefaultTypes[tileLabel] = typeName;
                refreshTypeDefaultTooltips();
            });
        };

        addDefaultAction(tileMenu, QStringLiteral("Shim"));
        addDefaultAction(tileMenu, QStringLiteral("Memory"));
        addDefaultAction(tileMenu, QStringLiteral("Compute"));
        addDefaultAction(allMenu, QStringLiteral("All Tiles"));
        addDefaultAction(allMenu, QStringLiteral("All Operators"));
        addDefaultAction(allMenu, QStringLiteral("All Nodes"));

        menu.exec(m_symbolTypeTable->viewport()->mapToGlobal(pos));
    }

    void addDesignNode()
    {
        if (!m_designModel)
            return;

        if (m_designRootPath.isEmpty()) {
            QMessageBox::information(this, tr("Select Workspace"),
                                     tr("Choose a workspace folder before creating a design."));
            openDesignFolder();
            if (m_designRootPath.isEmpty())
                return;
        }

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Create Design"));
        auto *layout = new QVBoxLayout(&dlg);
        auto *form = new QFormLayout;

        auto *nameEdit = new QLineEdit(&dlg);
        QString defaultName;
        int counter = m_designCounter;
        do {
            defaultName = QStringLiteral("design%1.aiecad").arg(counter++);
        } while (designNameExists(defaultName));
        nameEdit->setText(defaultName);

        auto *npuRow = new QWidget(&dlg);
        auto *npuLayout = new QHBoxLayout(npuRow);
        npuLayout->setContentsMargins(0, 0, 0, 0);
        npuLayout->setSpacing(8);
        auto *npuV1 = new QRadioButton(tr("v1"), npuRow);
        auto *npuV2 = new QRadioButton(tr("v2"), npuRow);
        npuV2->setChecked(true);
        npuLayout->addWidget(new QLabel(tr("NPU:"), npuRow));
        npuLayout->addWidget(npuV1);
        npuLayout->addWidget(npuV2);
        npuLayout->addStretch(1);

        auto *populateGrid = new QCheckBox(tr("Populate grid with tiles"), &dlg);

        form->addRow(tr("Name"), nameEdit);
        form->addRow(npuRow);
        form->addRow(populateGrid);
        layout->addLayout(form);

        auto *buttonsRow = new QHBoxLayout;
        buttonsRow->addStretch(1);
        auto *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
        cancelBtn->setStyleSheet(QStringLiteral("QPushButton { background: #c62828; color: white; }"));
        auto *createBtn = new QPushButton(tr("Create"), &dlg);
        createBtn->setDefault(true);
        buttonsRow->addWidget(cancelBtn);
        buttonsRow->addWidget(createBtn);
        layout->addLayout(buttonsRow);

        auto validate = [this, nameEdit, createBtn]() {
            const QString text = nameEdit->text().trimmed();
            bool ok = !text.isEmpty() && !designNameExists(text);
            if (ok && !m_designRootPath.isEmpty()) {
                QDir root(m_designRootPath);
                ok = !root.exists(text);
            }
            createBtn->setEnabled(ok);
        };
        validate();

        connect(nameEdit, &QLineEdit::textChanged, &dlg, validate);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(createBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

        if (dlg.exec() == QDialog::Accepted) {
            const QString name = nameEdit->text().trimmed();
            m_designCounter = counter;
            const QString npuVersion = npuV1->isChecked() ? QStringLiteral("v1") : QStringLiteral("v2");
            QDir root(m_designRootPath);
            if (!root.mkpath(name)) {
                QMessageBox::warning(this, tr("Create Failed"),
                                     tr("Could not create folder \"%1\".").arg(name));
                return;
            }
            // Create metadata stub file inside the design folder.
            const QString designDir = root.filePath(name);
            const QString metaPath = metadataFilePath(designDir);
            QJsonObject stub;
            stub["device"] = (npuVersion == QStringLiteral("v1")) ? QStringLiteral("npu1") : QStringLiteral("npu2");
            stub["variables"] = QJsonArray{};
            stub["types"] = QJsonArray{};
            stub["nodes"] = QJsonArray{};
            stub["connections"] = QJsonArray{};
            stub["object_fifos"] = QJsonArray{};
            stub["populate_grid"] = populateGrid->isChecked();
            QFile metaFile(metaPath);
            if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QMessageBox::warning(this, tr("Create Failed"),
                                     tr("Could not create metadata file for \"%1\".").arg(name));
                return;
            }
            metaFile.write(QJsonDocument(stub).toJson(QJsonDocument::Indented));
            metaFile.close();
            appendDesign(name, npuVersion);
        }
    }

    void removeSelectedDesignRoot()
    {
        if (!m_designTree || !m_designModel)
            return;

        const QModelIndex idx = m_designTree->currentIndex();
        if (!idx.isValid())
            return;

        QModelIndex rootIdx = idx;
        if (idx.parent().isValid())
            rootIdx = idx.parent();

        const QString name = m_designModel->itemFromIndex(rootIdx)->text();
        const auto reply = QMessageBox::question(this, tr("Remove Design"),
                                                 tr("Delete \"%1\" and its generated files?").arg(name),
                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;

        bool removed = true;
        if (!m_designRootPath.isEmpty()) {
            QDir root(m_designRootPath);
            const QString path = root.filePath(name);
            if (QDir(path).exists()) {
                removed = QDir(path).removeRecursively();
                if (!removed) {
                    QMessageBox::warning(this, tr("Delete Failed"),
                                         tr("Could not delete \"%1\" from disk.").arg(path));
                }
            }
        }

        if (removed) {
            m_designModel->removeRow(rootIdx.row());
            if (QDir(m_designRootPath).filePath(name) == m_activeDesignPath)
                setCanvasInactive(tr("Select/create an AIECAD design to open it"));
        }
        if (!m_designModel || m_designModel->rowCount() == 0)
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
    }

    void openDesignFolder()
    {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Design Folder"), m_designRootPath);
        if (dir.isEmpty())
            return;

        loadDesignFolder(dir);
    }

    void addSymbolTypeRow()
    {
        if (!m_symbolTypeTable)
            return;

        const int row = m_symbolTypeTable->rowCount();
        m_symbolTypeTable->insertRow(row);
        m_symbolTypeTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("type%1").arg(row + 1)));

        const auto vars = symbolVariableNames();
        const QString defaultDims = vars.isEmpty() ? QString() : QStringLiteral("$%1").arg(vars.front());
        m_symbolTypeTable->setItem(row, 1, new QTableWidgetItem(defaultDims));
        attachTypeCombo(row, QStringLiteral("int32"));
        m_symbolTypeTable->selectRow(row);
        updateTypeDimsValidation();
        refreshTypeDefaultTooltips();
        updateFifoTypeOptions();
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

    void removeSymbolTypeRow()
    {
        if (!m_symbolTypeTable)
            return;

        int row = m_symbolTypeTable->currentRow();
        if (row < 0)
            row = m_symbolTypeTable->rowCount() - 1;

        if (row >= 0)
            m_symbolTypeTable->removeRow(row);

        if (m_symbolTypeTable->rowCount() > 0)
            m_symbolTypeTable->selectRow(std::min(row, m_symbolTypeTable->rowCount() - 1));

        updateTypeDimsValidation();
        refreshTypeDefaultTooltips();
        updateFifoTypeOptions();
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

private Q_SLOTS:
    void onAddInPort()
    {
        const auto bases = currentBaseNodes();
        for (auto *base : bases) {
            const auto pc = constraintsFor(base);
            if (!pc.allowIn)
                continue;
            base->addInPort();
        }

        refreshSelectedNodeGeometries();

        if (auto *base = currentBaseNode())
            refreshPortCounts(base);
    }

    void onRemoveInPort()
    {
        const auto bases = currentBaseNodes();
        for (auto *base : bases) {
            const auto pc = constraintsFor(base);
            if (!pc.allowIn)
                continue;
            if (base->nPorts(PortType::In) > pc.minIn)
                base->removeInPort();
        }

        refreshSelectedNodeGeometries();

        if (auto *base = currentBaseNode())
            refreshPortCounts(base);
    }

    void onAddOutPort()
    {
        const auto bases = currentBaseNodes();
        for (auto *base : bases) {
            const auto pc = constraintsFor(base);
            if (!pc.allowOut)
                continue;
            base->addOutPort();
        }

        refreshSelectedNodeGeometries();

        if (auto *base = currentBaseNode())
            refreshPortCounts(base);
        if (auto *base = currentBaseNode(); base && (base->name() == QLatin1String("EntryNode") ||
                                                     base->name() == QLatin1String("OutputNode")))
            updatePropertiesPanel();
    }

    void onRemoveOutPort()
    {
        const auto bases = currentBaseNodes();
        for (auto *base : bases) {
            const auto pc = constraintsFor(base);
            if (!pc.allowOut)
                continue;
            if (base->nPorts(PortType::Out) > pc.minOut)
                base->removeOutPort();
        }

        refreshSelectedNodeGeometries();

        if (auto *base = currentBaseNode())
            refreshPortCounts(base);
        if (auto *base = currentBaseNode(); base && (base->name() == QLatin1String("EntryNode") ||
                                                     base->name() == QLatin1String("OutputNode")))
            updatePropertiesPanel();
    }

    void onGenerateCode()
    {
        if (m_activeDesignPath.isEmpty()) {
            QMessageBox::warning(this, tr("Generate Code"), tr("No active design selected."));
            return;
        }

        if (!m_loadingMetadata)
            saveActiveDesignMetadata();

        const QString metaPath = metadataFilePath(m_activeDesignPath);
        if (!QFileInfo::exists(metaPath)) {
            QMessageBox::warning(this, tr("Generate Code"),
                                 tr("Metadata file not found for the active design."));
            return;
        }

        const QString venvPython = QStringLiteral("/Users/samer.ali/CLionProjects/aiecad-qt/venv/bin/python");
        if (!QFileInfo::exists(venvPython)) {
            QMessageBox::warning(this, tr("Generate Code"),
                                 tr("Python venv not found at %1").arg(venvPython));
            return;
        }

        QString helperScript;
        {
            QStringList candidates;
            QDir binDir(QCoreApplication::applicationDirPath());
            QDir projDir = binDir;
            projDir.cdUp(); // cmake-build-debug
            projDir.cdUp(); // project root
            candidates << projDir.filePath(QStringLiteral("src/plugins/core/aiecad_compiler/tools/metadata_to_hlir.py"));
            candidates << QDir::current().absoluteFilePath(
                QStringLiteral("src/plugins/core/aiecad_compiler/tools/metadata_to_hlir.py"));
            for (const auto &c : candidates) {
                if (QFileInfo::exists(c)) {
                    helperScript = c;
                    break;
                }
            }
        }

        if (helperScript.isEmpty()) {
            QMessageBox::warning(this, tr("Generate Code"),
                                 tr("Helper script not found in source tree."));
            return;
        }

        auto *dlg = new QDialog(this);
        dlg->setWindowTitle(tr("Generate Code Output"));
        dlg->resize(700, 500);
        auto *layout = new QVBoxLayout(dlg);
        auto *log = new QTextEdit(dlg);
        log->setReadOnly(true);
        layout->addWidget(log, 1);
        auto *closeBtn = new QPushButton(tr("Close"), dlg);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
        connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("PYTHONPATH"),
                   QDir::current().absoluteFilePath(QStringLiteral("src/plugins/core/aiecad_compiler")));
        proc.setProcessEnvironment(env);
        QStringList args {
            helperScript,
            metaPath,
            QStringLiteral("--emit-gui"),
            QStringLiteral("--emit-complete"),
            QStringLiteral("--emit-graphml"),
            QStringLiteral("--emit-code")
        };

        proc.setProgram(venvPython);
        proc.setArguments(args);
        proc.setProcessChannelMode(QProcess::MergedChannels);

        proc.start();
        if (!proc.waitForStarted()) {
            log->append(tr("Failed to start process: %1").arg(proc.errorString()));
        } else {
            while (proc.state() != QProcess::NotRunning) {
                proc.waitForReadyRead(100);
                log->append(QString::fromUtf8(proc.readAllStandardOutput()));
            }
        }
        proc.waitForFinished(-1);
        log->append(tr("\nProcess exited with code %1").arg(proc.exitCode()));
        dlg->exec();
    }

    void showNodeContextMenu(NodeId nodeId, QPointF scenePos)
    {
        if (!m_editor || !m_editor->graphModel() || !m_editor->scene())
            return;

        auto *model = m_editor->graphModel()->delegateModel<NodeDelegateModel>(nodeId);
        auto *base  = dynamic_cast<BaseNodeModel *>(model);

        QMenu menu(this);

        QAction *addIn  = menu.addAction(tr("Add Input Port"));
        QAction *addOut = menu.addAction(tr("Add Output Port"));
        QAction *remIn  = menu.addAction(tr("Remove Input Port"));
        QAction *remOut = menu.addAction(tr("Remove Output Port"));

        const auto pc = constraintsFor(base);

        addIn->setEnabled(pc.allowIn);
        addOut->setEnabled(pc.allowOut);
        remIn->setEnabled(pc.allowIn && base && base->nPorts(PortType::In) > pc.minIn);
        remOut->setEnabled(pc.allowOut && base && base->nPorts(PortType::Out) > pc.minOut);

        connect(addIn,  &QAction::triggered, this, [this, base, nodeId, pc]() {
            if (base && pc.allowIn) {
                base->addInPort();
                if (m_selectedNode && *m_selectedNode == nodeId)
                    refreshPortCounts(base);
                refreshNodeGeometry(nodeId);
            }
        });
        connect(addOut, &QAction::triggered, this, [this, base, nodeId, pc]() {
            if (base && pc.allowOut) {
                base->addOutPort();
                if (m_selectedNode && *m_selectedNode == nodeId)
                    refreshPortCounts(base);
                refreshNodeGeometry(nodeId);
            }
        });
        connect(remIn,  &QAction::triggered, this, [this, base, nodeId, pc]() {
            if (base && pc.allowIn && base->nPorts(PortType::In) > pc.minIn) {
                base->removeInPort();
                if (m_selectedNode && *m_selectedNode == nodeId)
                    refreshPortCounts(base);
                refreshNodeGeometry(nodeId);
            }
        });
        connect(remOut, &QAction::triggered, this, [this, base, nodeId, pc]() {
            if (base && pc.allowOut && base->nPorts(PortType::Out) > pc.minOut) {
                base->removeOutPort();
                if (m_selectedNode && *m_selectedNode == nodeId)
                    refreshPortCounts(base);
                refreshNodeGeometry(nodeId);
            }
        });

        QPoint globalPos = m_editor->view()
                               ? m_editor->view()->mapToGlobal(m_editor->view()->mapFromScene(scenePos))
                               : QCursor::pos();

        menu.exec(globalPos);
    }

    void onConnectionCreated(const ConnectionId &connectionId)
    {
        // Entry nodes may only connect to Shim tiles.
        if (m_editor && m_editor->graphModel()) {
            auto *outDelegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(connectionId.outNodeId);
            auto *inDelegate  = m_editor->graphModel()->delegateModel<NodeDelegateModel>(connectionId.inNodeId);
            const QString outName = outDelegate ? outDelegate->name() : QString();
            const QString inName  = inDelegate ? inDelegate->name() : QString();
            if (outName == QLatin1String("EntryNode") && inName != QLatin1String("ShimTile")) {
                QMessageBox::warning(this, tr("Invalid Connection"),
                                     tr("Entry nodes may only connect to Shim tiles."));
                m_editor->graphModel()->deleteConnection(connectionId);
                return;
            }
        }

        EdgeProperties edge;
        edge.typeName = defaultSymbolType();
        edge.name = nextDefaultConnectionName();
        m_connectionProps.emplace(connectionId, edge);
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();

        if (m_selectedNode && m_editor && m_editor->graphModel()) {
            auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(*m_selectedNode);
            if (delegate && delegate->name() == QLatin1String("EntryNode"))
                updatePropertiesPanel();
            if (delegate && delegate->name() == QLatin1String("OutputNode"))
                updatePropertiesPanel();
        }
    }

    void onConnectionDeleted(const ConnectionId &connectionId)
    {
        m_connectionProps.erase(connectionId);
        if (m_selectedConnection && *m_selectedConnection == connectionId)
            m_selectedConnection.reset();
        if (!m_selectedConnections.empty()) {
            m_selectedConnections.erase(
                std::remove(m_selectedConnections.begin(), m_selectedConnections.end(), connectionId),
                m_selectedConnections.end());
        }
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
        if (m_editor && m_editor->scene())
            m_editor->scene()->update();

        if (m_selectedNode && m_editor && m_editor->graphModel()) {
            auto *delegate = m_editor->graphModel()->delegateModel<NodeDelegateModel>(*m_selectedNode);
            if (delegate && (delegate->name() == QLatin1String("EntryNode") ||
                             delegate->name() == QLatin1String("OutputNode")))
                updatePropertiesPanel();
        }
    }

    void onGraphModelReset()
    {
        m_connectionProps.clear();
        m_selectedConnection.reset();
        m_selectedConnections.clear();
        m_nodeGridCoords.clear();
        updatePropertiesPanel();
    }

    void restoreSettings()
    {
        QSettings settings(QStringLiteral("AIECAD"), QStringLiteral("CorePlugin"));
        const QString dir = settings.value(QStringLiteral("designRootPath")).toString();
        if (!dir.isEmpty())
            loadDesignFolder(dir);
    }

    void onDesignSelectionChanged(const QItemSelection &selected, const QItemSelection &)
    {
        if (!m_designTree || !m_designModel)
            return;

        if (selected.indexes().isEmpty()) {
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }

        QModelIndex idx = selected.indexes().first();
        if (idx.parent().isValid())
            idx = idx.parent();

        auto *item = m_designModel->itemFromIndex(idx);
        if (!item) {
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }

        const QString name = item->text();
        if (m_designRootPath.isEmpty()) {
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }

        const QString designDir = QDir(m_designRootPath).filePath(name);
        const QString metaPath = metadataFilePath(designDir);
        if (!QFileInfo::exists(metaPath)) {
            QMessageBox::warning(this, tr("Missing Metadata"),
                                 tr("No .aiecad metadata file found for \"%1\".\nCreate one to open this design.")
                                     .arg(name));
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }

        m_activeDesignPath = designDir;
        loadDesignMetadata(metaPath);
    }

    void onDesignDoubleClicked(const QModelIndex &index)
    {
        if (!index.isValid() || !m_designModel || m_designRootPath.isEmpty())
            return;

        auto showFile = [this](const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("Open File"),
                                     tr("Unable to open %1").arg(path));
                return;
            }
            const QString content = QString::fromUtf8(file.readAll());
            auto *dlg = new QDialog(this);
            dlg->setWindowTitle(QFileInfo(path).fileName());
            dlg->resize(800, 600);
            auto *layout = new QVBoxLayout(dlg);
            auto *editor = new QPlainTextEdit(dlg);
            editor->setReadOnly(true);
            editor->setPlainText(content);
            layout->addWidget(editor);
            auto *closeBtn = new QPushButton(tr("Close"), dlg);
            connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
            layout->addWidget(closeBtn, 0, Qt::AlignRight);
            dlg->exec();
        };

        if (index.parent().isValid()) {
            // Child item: show read-only viewer
            const QString designName = m_designModel->itemFromIndex(index.parent())->text();
            const QString fileName = m_designModel->itemFromIndex(index)->text();
            const QString path = QDir(QDir(m_designRootPath).filePath(designName)).filePath(fileName);
            showFile(path);
            return;
        }

        QModelIndex rootIdx = index;

        if (rootIdx != m_designTree->currentIndex())
            m_designTree->setCurrentIndex(rootIdx);

        const QString name = m_designModel->itemFromIndex(rootIdx)->text();
        const QString designDir = QDir(m_designRootPath).filePath(name);
        const QString metaPath = metadataFilePath(designDir);
        if (!QFileInfo::exists(metaPath)) {
            QMessageBox::warning(this, tr("Missing Metadata"),
                                 tr("No .aiecad metadata file found for \"%1\".\nCreate one to open this design.")
                                     .arg(name));
            setCanvasInactive(tr("Select/create an AIECAD design to open it"));
            return;
        }

        m_activeDesignPath = designDir;
        loadDesignMetadata(metaPath);
    }

    void onDepthChanged(int value)
    {
        updateObjectFifoValidation();
        if (!m_selectedConnections.empty()) {
            for (const auto &cid : m_selectedConnections) {
                auto &edge = m_connectionProps[cid];
                edge.depth = value;
            }
        } else if (m_selectedConnection) {
            auto &edge = m_connectionProps[*m_selectedConnection];
            edge.depth = value;
        }
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

    void onFifoTypeChanged(const QString &text)
    {
        if (!m_selectedConnections.empty()) {
            for (const auto &cid : m_selectedConnections) {
                auto &edge = m_connectionProps[cid];
                edge.typeName = text;
            }
        } else if (m_selectedConnection) {
            auto &edge = m_connectionProps[*m_selectedConnection];
            edge.typeName = text;
        }
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

    void onNameChanged(const QString &text)
    {
        if (!m_selectedConnections.empty()) {
            for (const auto &cid : m_selectedConnections) {
                auto &edge = m_connectionProps[cid];
                edge.name = text;
            }
        } else if (m_selectedConnection) {
            auto &edge = m_connectionProps[*m_selectedConnection];
            edge.name = text;
        }
        if (!m_loadingMetadata)
            saveActiveDesignMetadata();
    }

};

// -------------------------------------------------------------
// CorePlugin implementation
// -------------------------------------------------------------


CorePlugin::CorePlugin(QObject *parent)
    : IPlugin(parent)
{
    qInfo() << "[CorePlugin] Constructed";
}

CorePlugin::~CorePlugin()
{
    qInfo() << "[CorePlugin] Destructed";
    if (m_mainWindow)
        m_mainWindow->deleteLater();
}

bool CorePlugin::initialize(const QStringList &arguments,
                            QString &errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    qInfo() << "[CorePlugin] initialize()";

    // No services published yet; just report success.
    return true;
}

void CorePlugin::extensionsInitialized()
{
    qInfo() << "[CorePlugin] extensionsInitialized()";

    if (!m_mainWindow) {
        m_mainWindow = new CoreMainWindow;
        m_mainWindow->show();
    }
}

IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    qInfo() << "[CorePlugin] aboutToShutdown()";

    if (m_mainWindow) {
        m_mainWindow->close();
        m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
    }

    return IPlugin::ShutdownFlag::SynchronousShutdown;
}

} // namespace aiecad

#include "core.moc"
