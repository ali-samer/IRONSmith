#include "command/DeleteCommands.hpp"
#include "command/CommandError.hpp"

#include <designmodel/DesignDocument.hpp>

#include <algorithm>

namespace Command {

static CommandResult fail(CommandErrorCode code, const QString& msg)
{
    return CommandResult::failure(CommandError(code, msg));
}

template <typename T>
static void dedup(QVector<T>& v)
{
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
}

CommandResult DeleteEntitiesCommand::apply(const DesignModel::DesignDocument& input) const
{
    QVector<DesignModel::BlockId> blocks = m_blocks;
    QVector<DesignModel::LinkId> links = m_links;
    QVector<DesignModel::AnnotationId> annotations = m_annotations;
    QVector<DesignModel::NetId> nets = m_nets;
    QVector<DesignModel::RouteId> routes = m_routes;

    dedup(blocks);
    dedup(links);
    dedup(annotations);
    dedup(nets);
    dedup(routes);

    if (blocks.isEmpty() && links.isEmpty() && annotations.isEmpty() && nets.isEmpty() && routes.isEmpty())
        return fail(CommandErrorCode::InvalidArgument, "DeleteEntities: no ids provided.");

    DesignModel::DesignDocument::Builder b(input);

    bool removed = false;

    for (const auto id : routes)
        removed = b.removeRoute(id) || removed;

    for (const auto id : nets)
        removed = b.removeNet(id) || removed;

    for (const auto id : links)
        removed = b.removeLink(id) || removed;

    for (const auto id : blocks)
        removed = b.removeBlock(id) || removed;

    for (const auto id : annotations)
        removed = b.removeAnnotation(id) || removed;

    if (!removed)
        return fail(CommandErrorCode::MissingEntity, "DeleteEntities: nothing removed.");

    const auto out = b.freeze();
    return CommandResult::success(out);
}

} // namespace Command
