#include "canvas/CanvasControlsPanel.hpp"

#include "canvas/CanvasService.hpp"

#include <command/CommandDispatcher.hpp>
#include <command/BuiltInCommands.hpp>

#include <designmodel/Tile.hpp>
#include <designmodel/DesignEntities.hpp>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>

namespace Canvas {

CanvasControlsPanel::CanvasControlsPanel(QWidget* parent,
                                         CanvasService* service,
                                         Command::CommandDispatcher* dispatcher)
    : QWidget(parent)
    , m_service(service)
    , m_dispatcher(dispatcher)
{
    setObjectName("CanvasControlsPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* title = new QLabel(tr("Canvas Controls"), this);
    QFont f = title->font();
    f.setPointSizeF(f.pointSizeF() + 2);
    f.setWeight(QFont::DemiBold);
    title->setFont(f);
    root->addWidget(title);

    auto* form = new QFormLayout();
    form->setFormAlignment(Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);

    m_col = new QSpinBox(this);
    m_col->setRange(0, 255);
    m_col->setValue(0);
    form->addRow(tr("Col"), m_col);

    m_row = new QSpinBox(this);
    m_row->setRange(0, 255);
    m_row->setValue(0);
    form->addRow(tr("Row"), m_row);

    root->addLayout(form);

    m_place = new QPushButton(tr("Place Compute"), this);
    root->addWidget(m_place);
    connect(m_place, &QPushButton::clicked, this, &CanvasControlsPanel::onPlaceCompute);

    m_showAnnotations = new QCheckBox(tr("Show Annotations"), this);
    m_showAnnotations->setChecked(m_service ? m_service->renderOptions().showAnnotations : true);
    root->addWidget(m_showAnnotations);
    connect(m_showAnnotations, &QCheckBox::toggled, this, &CanvasControlsPanel::onToggleAnnotations);

    m_showFabric = new QCheckBox(tr("Show Fabric"), this);
    m_showFabric->setChecked(m_service ? m_service->renderOptions().showFabric : true);
    root->addWidget(m_showFabric);
    connect(m_showFabric, &QCheckBox::toggled, this, &CanvasControlsPanel::onToggleFabric);

    m_showPortHotspots = new QCheckBox(tr("Show Port Hotspots"), this);
    m_showPortHotspots->setChecked(m_service ? m_service->renderOptions().showPortHotspots : true);
    root->addWidget(m_showPortHotspots);
    connect(m_showPortHotspots, &QCheckBox::toggled, this, &CanvasControlsPanel::onTogglePortHotspots);

    root->addStretch(1);
}

void CanvasControlsPanel::onPlaceCompute()
{
    if (!m_dispatcher)
        return;

    const int col = m_col ? m_col->value() : 0;
    const int row = m_row ? m_row->value() : 0;

    DesignModel::TileCoord coord(row, col);

    m_dispatcher->beginTransaction("PlaceComputeWithPorts");

    Command::CreateBlockCommand createBlock(DesignModel::BlockType::Compute, DesignModel::Placement(coord), "AIE");
    const auto r = m_dispatcher->apply(createBlock);
    if (!r.ok() || !r.payload().canConvert<Command::CreatedBlock>()) {
        m_dispatcher->rollbackTransaction();
        return;
    }

    const auto payload = r.payload().value<Command::CreatedBlock>();
    const DesignModel::BlockId bid = payload.id;

    const DesignModel::PortType stream(DesignModel::PortTypeKind::Stream, "objfifo");
    struct Spec { const char* name; DesignModel::PortDirection dir; };
    const Spec ports[] = {
        {"port_N", DesignModel::PortDirection::Input},
        {"port_W", DesignModel::PortDirection::Input},
        {"port_E", DesignModel::PortDirection::Output},
        {"port_S", DesignModel::PortDirection::Output},
    };

    for (const auto& sp : ports) {
        Command::CreatePortCommand createPort(bid, sp.dir, stream, sp.name, 1);
        const auto pr = m_dispatcher->apply(createPort);
        if (!pr.ok()) {
            m_dispatcher->rollbackTransaction();
            return;
        }
    }

    m_dispatcher->commitTransaction();
}

void CanvasControlsPanel::onToggleAnnotations(bool checked)
{
    if (!m_service)
        return;

    auto opts = m_service->renderOptions();
    opts.showAnnotations = checked;
    m_service->setRenderOptions(opts);
}

void CanvasControlsPanel::onToggleFabric(bool checked)
{
    if (!m_service)
        return;
    auto opts = m_service->renderOptions();
    opts.showFabric = checked;
    m_service->setRenderOptions(opts);
}

void CanvasControlsPanel::onTogglePortHotspots(bool checked)
{
    if (!m_service)
        return;
    auto opts = m_service->renderOptions();
    opts.showPortHotspots = checked;
    m_service->setRenderOptions(opts);
}

} // namespace Canvas