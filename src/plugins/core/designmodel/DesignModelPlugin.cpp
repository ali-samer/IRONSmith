#include "DesignModelPlugin.hpp"

#include "designmodel/DesignId.hpp"
#include "designmodel/Tile.hpp"
#include "designmodel/DesignSchemaVersion.hpp"

#include <QtCore/QMetaType>

#include "extensionsystem/PluginManager.hpp"

namespace DesignModel::Internal {

static void registerDesignModelMetaTypes()
{
	qRegisterMetaType<BlockId>("DesignModel::BlockId");
	qRegisterMetaType<PortId>("DesignModel::PortId");
	qRegisterMetaType<LinkId>("DesignModel::LinkId");
	qRegisterMetaType<NetId>("DesignModel::NetId");
	qRegisterMetaType<AnnotationId>("DesignModel::AnnotationId");
	qRegisterMetaType<RouteId>("DesignModel::RouteId");
	qRegisterMetaType<TileCoord>("DesignModel::TileCoord");
	qRegisterMetaType<TileKind>("DesignModel::TileKind");
	qRegisterMetaType<DesignSchemaVersion>("DesignModel::DesignSchemaVersion");
}

Utils::Result DesignModelPlugin::initialize(const QStringList& /*arguments*/, ExtensionSystem::PluginManager& /*pluginManager*/)
{
	registerDesignModelMetaTypes();
	return Utils::Result::success();
}

void DesignModelPlugin::extensionsInitialized(ExtensionSystem::PluginManager&)
{
}

ExtensionSystem::IPlugin::ShutdownFlag DesignModelPlugin::aboutToShutdown()
{
	return ShutdownFlag::SynchronousShutdown;
}

} // namespace DesignModel::Internal
