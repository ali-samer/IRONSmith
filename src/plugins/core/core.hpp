#pragma once

#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>

#include "IPlugin.hpp"

namespace aiecad {

class CoreMainWindow; // defined in core.cpp

class CorePlugin : public IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.aiecad.CorePlugin" FILE "Core.json")

public:
	explicit CorePlugin(QObject *parent = nullptr);
	~CorePlugin() override;

	bool initialize(const QStringList &arguments,
					QString &errorString) override;

	void extensionsInitialized() override;
	ShutdownFlag aboutToShutdown() override;

private:
	QPointer<CoreMainWindow> m_mainWindow;
};

} // namespace aiecad