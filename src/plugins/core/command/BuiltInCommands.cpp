#include "command/BuiltInCommands.hpp"
#include "command/CommandError.hpp"

#include <designmodel/DesignDocument.hpp>

namespace Command {

static CommandResult fail(CommandErrorCode code, const QString& msg)
{
    return CommandResult::failure(CommandError(code, msg));
}

CommandResult CreateBlockCommand::apply(const DesignModel::DesignDocument& input) const
{
    if (m_type == DesignModel::BlockType::Unknown)
        return fail(CommandErrorCode::InvalidArgument, "CreateBlock: BlockType is Unknown.");
    if (!m_placement.isValid())
        return fail(CommandErrorCode::InvalidArgument, "CreateBlock: Placement is invalid.");

    DesignModel::DesignDocument::Builder b(input);
    const auto id = b.createBlock(m_type, m_placement, m_displayName);
    const auto out = b.freeze();

    CreatedBlock payload{id};
    return CommandResult::success(out, QVariant::fromValue(payload));
}

CommandResult CreatePortCommand::apply(const DesignModel::DesignDocument& input) const
{
    if (m_owner.isNull())
        return fail(CommandErrorCode::InvalidArgument, "CreatePort: owner is null.");
    if (!input.tryBlock(m_owner))
        return fail(CommandErrorCode::MissingEntity, "CreatePort: owner block does not exist.");
    if (!m_type.isValid())
        return fail(CommandErrorCode::InvalidArgument, "CreatePort: PortType is invalid.");
    if (m_capacity < 1)
        return fail(CommandErrorCode::InvalidArgument, "CreatePort: capacity must be >= 1.");

    DesignModel::DesignDocument::Builder b(input);
    const auto id = b.createPort(m_owner, m_dir, m_type, m_name, m_capacity);
    const auto out = b.freeze();

    CreatedPort payload{id};
    return CommandResult::success(out, QVariant::fromValue(payload));
}

CommandResult CreateLinkCommand::apply(const DesignModel::DesignDocument& input) const
{
    if (m_from.isNull() || m_to.isNull())
        return fail(CommandErrorCode::InvalidArgument, "CreateLink: from/to is null.");
    if (m_from == m_to)
        return fail(CommandErrorCode::InvalidArgument, "CreateLink: from == to.");

    const auto* fromPort = input.tryPort(m_from);
    if (!fromPort)
        return fail(CommandErrorCode::MissingEntity, "CreateLink: from port does not exist.");
    const auto* toPort = input.tryPort(m_to);
    if (!toPort)
        return fail(CommandErrorCode::MissingEntity, "CreateLink: to port does not exist.");

    const auto fromDir = fromPort->direction();
    const auto toDir = toPort->direction();

    const bool fromOk = (fromDir == DesignModel::PortDirection::Output || fromDir == DesignModel::PortDirection::InOut);
    const bool toOk = (toDir == DesignModel::PortDirection::Input || toDir == DesignModel::PortDirection::InOut);

    if (!fromOk)
        return fail(CommandErrorCode::InvalidConnection, "CreateLink: from port is not an output.");
    if (!toOk)
        return fail(CommandErrorCode::InvalidConnection, "CreateLink: to port is not an input.");

    if (!(fromPort->type() == toPort->type())) {
        return fail(CommandErrorCode::InvalidConnection,
                    QString("CreateLink: PortType mismatch (%1 -> %2).")
                        .arg(static_cast<int>(fromPort->type().kind()))
                        .arg(static_cast<int>(toPort->type().kind())));
    }

    const int inCount = input.index().linksForPort(m_to).size();
    if (inCount >= toPort->capacity()) {
        return fail(CommandErrorCode::InvalidConnection,
                    QString("CreateLink: input port '%1' is at capacity (%2).")
                        .arg(toPort->name())
                        .arg(toPort->capacity()));
    }

    DesignModel::DesignDocument::Builder b(input);
    const auto id = b.createLink(m_from, m_to, m_label);
    const auto out = b.freeze();

    CreatedLink payload{id};
    return CommandResult::success(out, QVariant::fromValue(payload));
}

CommandResult AdjustLinkRouteCommand::apply(const DesignModel::DesignDocument& input) const
{
    if (m_linkId.isNull())
        return fail(CommandErrorCode::InvalidArgument, "AdjustLinkRoute: linkId is null.");

    const auto* link = input.tryLink(m_linkId);
    if (!link)
        return fail(CommandErrorCode::MissingEntity, "AdjustLinkRoute: link does not exist.");

    if (m_newOverride && !m_newOverride->isValid())
        return fail(CommandErrorCode::InvalidArgument, "AdjustLinkRoute: new override is invalid.");

    if (m_oldOverride != link->routeOverride())
        return fail(CommandErrorCode::InvariantViolation, "AdjustLinkRoute: stale base route.");

    if (m_newOverride == link->routeOverride())
        return fail(CommandErrorCode::InvalidArgument, "AdjustLinkRoute: no change.");

    DesignModel::DesignDocument::Builder b(input);
    if (!b.setLinkRouteOverride(m_linkId, m_newOverride))
        return fail(CommandErrorCode::Unknown, "AdjustLinkRoute: failed to apply.");

    const auto out = b.freeze();
    return CommandResult::success(out);
}

} // namespace Command
