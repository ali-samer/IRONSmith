#pragma once

#include <utils/EnvironmentQtPolicy.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

namespace Aie {
class AieCanvasCoordinator;
}

namespace Aie::Internal {

class AiePanelState final : public QObject
{
    Q_OBJECT

public:
    explicit AiePanelState(AieCanvasCoordinator* coordinator = nullptr, QObject* parent = nullptr);

    void setCoordinator(AieCanvasCoordinator* coordinator);

    static Utils::Environment makeEnvironment();

private:
    void loadState();
    void saveState();
    void scheduleSave();

    void apply(const QJsonObject& state);
    QJsonObject snapshot() const;

    QPointer<AieCanvasCoordinator> m_coordinator;
    Utils::Environment m_env;
    QTimer m_saveTimer;
    bool m_applying = false;
};

} // namespace Aie::Internal
