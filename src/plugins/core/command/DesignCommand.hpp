#pragma once

#include "command/CommandPluginGlobal.hpp"
#include "command/CommandResult.hpp"

#include <designmodel/DesignDocument.hpp>

#include <QtCore/QString>

namespace Command {

class COMMANDPLUGIN_EXPORT DesignCommand {
public:
	virtual ~DesignCommand() = default;
	virtual QString name() const = 0;
	virtual CommandResult apply(const DesignModel::DesignDocument& input) const = 0;
};

} // namespace Command