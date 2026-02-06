#pragma once

#include "aieplugin/AieGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include <utils/Result.hpp>

namespace Aie {

class AieCanvasCoordinator;

class AIEPLUGIN_EXPORT IAieHost : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~IAieHost() override = default;

    virtual AieCanvasCoordinator* coordinator() const = 0;
    virtual QString profileId() const = 0;
    virtual QStringList profileIds() const = 0;

    virtual Utils::Result loadProfileCatalog(const QString& path) = 0;
    virtual Utils::Result setProfileId(const QString& id) = 0;

signals:
    void profileIdChanged(const QString& id);
    void catalogChanged();
};

} // namespace Aie
