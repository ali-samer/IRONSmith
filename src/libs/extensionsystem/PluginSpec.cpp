#include "PluginSpec.hpp"

#include <QtCore/QObject>

namespace ExtensionSystem {

IPlugin* PluginSpec::instantiate()
{
	if (m_state == State::Failed)
		return nullptr;

	if (m_plugin)
		return m_plugin.data();

	if (!m_factory) {
		addError(QString("Plugin '%1' has no factory.").arg(m_id));
		return nullptr;
	}

	IPlugin* p = m_factory();
	if (!p) {
		addError(QString("Plugin '%1' factory returned null.").arg(m_id));
		return nullptr;
	}

	m_plugin = p;
	m_state = State::Instantiated;
	return p;
}

} // namespace ExtensionSystem