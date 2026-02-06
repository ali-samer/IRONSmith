#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <functional>

#include "IPlugin.hpp"

namespace ExtensionSystem {

class PluginSpec
{
public:
	enum class State {
		Discovered,
		Instantiated,
		Initialized,
		Failed
	};

	using Factory = std::function<IPlugin*()>;

	PluginSpec() = default;

	PluginSpec(QString id, QStringList dependencies, Factory factory)
		: m_id(std::move(id))
		, m_dependencies(std::move(dependencies))
		, m_factory(std::move(factory))
	{}

	const QString& id() const { return m_id; }
	const QStringList& dependencies() const { return m_dependencies; }

	bool hasError() const { return !m_errors.isEmpty(); }
	const QStringList& errors() const { return m_errors; }
	QString errorString() const { return m_errors.join('\n'); }

	void addError(const QString& msg)
	{
		if (!msg.isEmpty())
			m_errors.push_back(msg);
		m_state = State::Failed;
	}

	State state() const { return m_state; }

	bool isEffectivelyEnabled() const
	{
		return m_enabled && !hasError();
	}

	void setEnabled(bool on) { m_enabled = on; }

	IPlugin* plugin() const { return m_plugin.data(); }

	IPlugin* instantiate();

	void markInitialized() { m_state = State::Initialized; }

private:
	QString m_id;
	QStringList m_dependencies;
	Factory m_factory;

	bool m_enabled = true;

	QStringList m_errors;
	State m_state = State::Discovered;

	QPointer<IPlugin> m_plugin;
};

} // namespace ExtensionSystem
