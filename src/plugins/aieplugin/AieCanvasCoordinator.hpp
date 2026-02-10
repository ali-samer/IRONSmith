#pragma once

#include "aieplugin/NpuProfileCanvasMapper.hpp"

#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/api/ICanvasHost.hpp"
#include "canvas/api/ICanvasStyleHost.hpp"
#include "canvas/api/CanvasStyleTypes.hpp"
#include "utils/async/DebouncedInvoker.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtGui/QColor>
#include <memory>

namespace Aie {

class AieCanvasCoordinator final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double tileSpacing READ tileSpacing WRITE setTileSpacing NOTIFY tileSpacingChanged)
    Q_PROPERTY(double horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing NOTIFY horizontalSpacingChanged)
    Q_PROPERTY(double verticalSpacing READ verticalSpacing WRITE setVerticalSpacing NOTIFY verticalSpacingChanged)
    Q_PROPERTY(double outwardSpread READ outwardSpread WRITE setOutwardSpread NOTIFY outwardSpreadChanged)
    Q_PROPERTY(double outerMargin READ outerMargin WRITE setOuterMargin NOTIFY outerMarginChanged)
    Q_PROPERTY(bool autoCellSize READ autoCellSize WRITE setAutoCellSize NOTIFY autoCellSizeChanged)
    Q_PROPERTY(double cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(bool showPorts READ showPorts WRITE setShowPorts NOTIFY showPortsChanged)
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY showLabelsChanged)
    Q_PROPERTY(double keepoutMargin READ keepoutMargin WRITE setKeepoutMargin NOTIFY keepoutMarginChanged)
    Q_PROPERTY(bool useCustomColors READ useCustomColors WRITE setUseCustomColors NOTIFY useCustomColorsChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY outlineColorChanged)
    Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor NOTIFY labelColorChanged)

public:
    enum class SelectionSpacingAxis {
        Horizontal,
        Vertical,
        Outward
    };
    Q_ENUM(SelectionSpacingAxis)

    explicit AieCanvasCoordinator(QObject* parent = nullptr);
    ~AieCanvasCoordinator() override;

    void setCanvasHost(Canvas::Api::ICanvasHost* host);
    Canvas::Api::ICanvasHost* canvasHost() const { return m_canvasHost; }
    void setGridHost(Canvas::Api::ICanvasGridHost* host);
    Canvas::Api::ICanvasGridHost* gridHost() const { return m_gridHost; }
    void setStyleHost(Canvas::Api::ICanvasStyleHost* host);
    Canvas::Api::ICanvasStyleHost* styleHost() const { return m_styleHost; }

    void setBaseModel(const CanvasGridModel& model);
    const CanvasGridModel& baseModel() const { return m_baseModel; }

    void setBaseStyles(const QHash<QString, Canvas::Api::CanvasBlockStyle>& styles);
    QHash<QString, Canvas::Api::CanvasBlockStyle> baseStyles() const { return m_baseStyles; }

    double tileSpacing() const;
    void setTileSpacing(double spacing);

    double horizontalSpacing() const { return m_horizontalSpacing; }
    void setHorizontalSpacing(double spacing);

    double verticalSpacing() const { return m_verticalSpacing; }
    void setVerticalSpacing(double spacing);

    double outwardSpread() const { return m_outwardSpread; }
    void setOutwardSpread(double spread);

    double outerMargin() const { return m_outwardSpread; }
    void setOuterMargin(double margin);

    bool autoCellSize() const { return m_autoCellSize; }
    void setAutoCellSize(bool enabled);

    double cellSize() const { return m_cellSize; }
    void setCellSize(double size);

    bool showPorts() const { return m_showPorts; }
    void setShowPorts(bool enabled);

    bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool enabled);

    double keepoutMargin() const { return m_keepoutMargin; }
    void setKeepoutMargin(double margin);

    bool useCustomColors() const { return m_useCustomColors; }
    void setUseCustomColors(bool enabled);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color);

    QColor outlineColor() const { return m_outlineColor; }
    void setOutlineColor(const QColor& color);

    QColor labelColor() const { return m_labelColor; }
    void setLabelColor(const QColor& color);

    void apply();
    void beginSelectionSpacing(SelectionSpacingAxis axis);
    void updateSelectionSpacing(SelectionSpacingAxis axis, double value);
    void endSelectionSpacing(SelectionSpacingAxis axis);
    void nudgeSelection(double dx, double dy);

signals:
    void tileSpacingChanged(double spacing);
    void horizontalSpacingChanged(double spacing);
    void verticalSpacingChanged(double spacing);
    void outwardSpreadChanged(double spread);
    void outerMarginChanged(double margin);
    void autoCellSizeChanged(bool enabled);
    void cellSizeChanged(double size);
    void showPortsChanged(bool enabled);
    void showLabelsChanged(bool enabled);
    void keepoutMarginChanged(double margin);
    void useCustomColorsChanged(bool enabled);
    void fillColorChanged(const QColor& color);
    void outlineColorChanged(const QColor& color);
    void labelColorChanged(const QColor& color);

private:
    struct SelectionSnapshot;
    void requestApply();
    void applyNow();

    QPointer<Canvas::Api::ICanvasHost> m_canvasHost;
    QPointer<Canvas::Api::ICanvasGridHost> m_gridHost;
    QPointer<Canvas::Api::ICanvasStyleHost> m_styleHost;
    CanvasGridModel m_baseModel;
    QHash<QString, Canvas::Api::CanvasBlockStyle> m_baseStyles;
    QHash<QString, QPointF> m_blockOffsets;

    bool m_dirty = false;
    Utils::Async::DebouncedInvoker m_applyDebounce;

    double m_horizontalSpacing = 0.0;
    double m_verticalSpacing = 0.0;
    double m_outwardSpread = 0.0;
    bool m_autoCellSize = true;
    double m_cellSize = 0.0;
    bool m_showPorts = true;
    bool m_showLabels = true;
    double m_keepoutMargin = -1.0;

    bool m_useCustomColors = false;
    QColor m_fillColor;
    QColor m_outlineColor;
    QColor m_labelColor;

    std::unique_ptr<SelectionSnapshot> m_selectionSnapshot;
};

} // namespace Aie
