#pragma once

#include "command/DesignCommand.hpp"

#include <designmodel/DesignEntities.hpp>
#include <designmodel/DesignExtras.hpp>

namespace Command {

class DeleteEntitiesCommand final : public DesignCommand {
public:
    DeleteEntitiesCommand(QVector<DesignModel::BlockId> blocks = {},
                          QVector<DesignModel::LinkId> links = {},
                          QVector<DesignModel::AnnotationId> annotations = {},
                          QVector<DesignModel::NetId> nets = {},
                          QVector<DesignModel::RouteId> routes = {})
        : m_blocks(std::move(blocks))
        , m_links(std::move(links))
        , m_annotations(std::move(annotations))
        , m_nets(std::move(nets))
        , m_routes(std::move(routes)) {}

    QString name() const override { return QStringLiteral("DeleteEntities"); }
    CommandResult apply(const DesignModel::DesignDocument& input) const override;

private:
    QVector<DesignModel::BlockId> m_blocks;
    QVector<DesignModel::LinkId> m_links;
    QVector<DesignModel::AnnotationId> m_annotations;
    QVector<DesignModel::NetId> m_nets;
    QVector<DesignModel::RouteId> m_routes;
};

} // namespace Command
