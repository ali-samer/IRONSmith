#pragma once

#include "command/CommandPluginGlobal.hpp"
#include <QtCore/QString>

namespace Command {

enum class CommandErrorCode : quint8 {
	None = 0,
	InvalidArgument,
	MissingEntity,
	InvalidConnection,
	InvariantViolation,
	Unknown
};

class COMMANDPLUGIN_EXPORT CommandError final {
public:
	CommandError() = default;
	CommandError(CommandErrorCode code, QString message)
		: m_code(code), m_message(std::move(message)) {}

	bool ok() const noexcept { return m_code == CommandErrorCode::None; }
	CommandErrorCode code() const noexcept { return m_code; }
	const QString& message() const noexcept { return m_message; }

	static CommandError none() { return {}; }

private:
	CommandErrorCode m_code{CommandErrorCode::None};
	QString m_message;
};

} // namespace Command
