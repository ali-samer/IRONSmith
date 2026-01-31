#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "canvas/CanvasRenderOptions.hpp"
#include "canvas/CanvasView.hpp"

namespace Core { class IUiHost; class ISidebarRegistry; }
namespace Command { class CommandDispatcher; }

namespace Canvas {

class CanvasView;

class CanvasService final : public QObject
{
    Q_OBJECT

public:
    explicit CanvasService(QObject* parent = nullptr);
    ~CanvasService() override;

    const CanvasRenderOptions& renderOptions() const noexcept { return m_options; }
    void setRenderOptions(CanvasRenderOptions opts);

    CanvasView* view() const noexcept { return m_view; }

    void wireIntoApplication();

signals:
    void renderOptionsChanged(const Canvas::CanvasRenderOptions& opts);

private:
    void ensureInitialDocument();
    void registerSidebarTools();

    QPointer<Core::IUiHost> m_ui;
    QPointer<Core::ISidebarRegistry> m_sidebar;
    QPointer<Command::CommandDispatcher> m_dispatcher;
    QPointer<CanvasView> m_view;

    CanvasRenderOptions m_options{};
};

} // namespace Canvas
