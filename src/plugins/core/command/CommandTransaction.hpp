#pragma once

#include "command/CommandPluginGlobal.hpp"
#include "command/CommandDispatcher.hpp"

#include <QtCore/QString>

namespace Command {

class COMMANDPLUGIN_EXPORT CommandTransaction final {
public:
	CommandTransaction(CommandDispatcher& dispatcher, QString label = {});
	~CommandTransaction();

	CommandTransaction(const CommandTransaction&) = delete;
	CommandTransaction& operator=(const CommandTransaction&) = delete;

	CommandTransaction(CommandTransaction&&) = delete;
	CommandTransaction& operator=(CommandTransaction&&) = delete;

	void commit();
	void rollback();

	bool isActive() const noexcept { return m_active; }
	bool wasRolledBack() const noexcept { return m_rolledBack; }
	const QString& label() const noexcept { return m_label; }

private:
	CommandDispatcher& m_dispatcher;
	QString m_label;
	bool m_active{true};
	bool m_rolledBack{false};
};

} // namespace Command