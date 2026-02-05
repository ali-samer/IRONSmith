#include "command/CommandTransaction.hpp"

namespace Command {

CommandTransaction::CommandTransaction(CommandDispatcher& dispatcher, QString label)
	: m_dispatcher(dispatcher)
	, m_label(std::move(label))
{
	m_dispatcher.beginTransaction(m_label);
}

CommandTransaction::~CommandTransaction()
{
	if (!m_active)
		return;

	m_dispatcher.commitTransaction();
	m_active = false;
}

void CommandTransaction::commit()
{
	if (!m_active)
		return;

	m_dispatcher.commitTransaction();
	m_active = false;
}

void CommandTransaction::rollback()
{
	if (!m_active)
		return;

	m_dispatcher.rollbackTransaction();
	m_active = false;
	m_rolledBack = true;
}

} // namespace Command