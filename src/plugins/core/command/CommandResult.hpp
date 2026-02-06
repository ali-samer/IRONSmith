#pragma once

#include "command/CommandPluginGlobal.hpp"
#include "command/CommandError.hpp"

#include <designmodel/DesignDocument.hpp>

#include <QtCore/QVariant>

namespace Command {

class COMMANDPLUGIN_EXPORT CommandResult final {
public:
	CommandResult() = default;

	static CommandResult success(DesignModel::DesignDocument doc, QVariant payload = {})
	{
		CommandResult r;
		r.m_ok = true;
		r.m_document = std::move(doc);
		r.m_payload = std::move(payload);
		return r;
	}

	static CommandResult failure(CommandError err)
	{
		CommandResult r;
		r.m_ok = false;
		r.m_error = std::move(err);
		return r;
	}

	bool ok() const noexcept { return m_ok; }
	const CommandError& error() const noexcept { return m_error; }
	const DesignModel::DesignDocument& document() const noexcept { return m_document; }
	const QVariant& payload() const noexcept { return m_payload; }

private:
	bool m_ok{false};
	CommandError m_error{CommandError::none()};
	DesignModel::DesignDocument m_document{};
	QVariant m_payload{};
};

} // namespace Command

Q_DECLARE_METATYPE(Command::CommandResult)
