#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include "extensionsystem/PluginManager.hpp"
#include "extensionsystem/PluginSpec.hpp"

#include "core/CorePlugin.hpp"
// Bridge tests are in tests/hlir_bridge/
#include "BridgeTests.hpp"

using namespace ExtensionSystem;

static constexpr char corePluginIdC[] = "core";

// Temporary plugin registration.
// Replace JSON-based plugin discovery
static void registerSystemPlugins()
{
	PluginManager::registerPlugin(
		PluginSpec(QString::fromLatin1(corePluginIdC),
				   {},
				   []() -> IPlugin* { return new Core::CorePlugin; })
	);
}

static void printErrorsAndFail(const QString& header, const QStringList& errors)
{
	qCritical().noquote() << header;
	for (const QString& e : errors)
		qCritical().noquote() << "  " << e;
}

int main(int argc, char** argv)
{
	QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);

	QApplication app(argc, argv);

	// Check for bridge test command-line argument
	QStringList args = app.arguments();
	if (args.contains("--test-bridges") || args.contains("-t")) {
		bool testsPassed = BridgeTests::runBridgeTests();
		return testsPassed ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	registerSystemPlugins();

	PluginSpec* core = PluginManager::specById(corePluginIdC);
	if (!core) {
		printErrorsAndFail("Core plugin spec not found.", PluginManager::lastErrors());
		return EXIT_FAILURE;
	}
	if (!core->isEffectivelyEnabled()) {
		printErrorsAndFail("Core plugin is not enabled.", core->errors());
		return EXIT_FAILURE;
	}
	if (core->hasError()) {
		printErrorsAndFail("Core plugin spec has errors.", core->errors());
		return EXIT_FAILURE;
	}

	if (!PluginManager::loadPlugins(app.arguments())) {
		printErrorsAndFail("Failed to load plugins.", PluginManager::lastErrors());
		return EXIT_FAILURE;
	}

	if (core->hasError()) {
		printErrorsAndFail("Core plugin failed during load.", core->errors());
		return EXIT_FAILURE;
	}

	return app.exec();
}