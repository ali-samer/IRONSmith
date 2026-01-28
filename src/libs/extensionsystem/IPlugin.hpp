#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <utils/Result.hpp>

namespace ExtensionSystem {

class PluginManager;

class IPlugin : public QObject
{
	Q_OBJECT

public:
	enum class ShutdownFlag {
		SynchronousShutdown,
		AsynchronousShutdown
	};
	Q_ENUM(ShutdownFlag)

	explicit IPlugin(QObject* parent = nullptr) : QObject(parent) {}
	~IPlugin() override = default;

	virtual Utils::Result initialize(const QStringList& arguments, PluginManager& manager) = 0;

	virtual void extensionsInitialized(PluginManager& /*manager*/) {}

	virtual bool delayedInitialize() { return false; }

	virtual ShutdownFlag aboutToShutdown() { return ShutdownFlag::SynchronousShutdown; }

	signals:
		void asynchronousShutdownFinished();
};

} // namespace ExtensionSystem