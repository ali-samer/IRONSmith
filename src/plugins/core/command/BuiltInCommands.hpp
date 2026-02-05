#pragma once

#include "command/DesignCommand.hpp"

#include <designmodel/DesignEntities.hpp>

#include <optional>

namespace Command {

struct CreatedBlock {
    DesignModel::BlockId id{};
};

struct CreatedPort {
    DesignModel::PortId id{};
};

struct CreatedLink {
    DesignModel::LinkId id{};
};

class CreateBlockCommand final : public DesignCommand {
public:
    CreateBlockCommand(DesignModel::BlockType type,
                       DesignModel::Placement placement,
                       QString displayName = {})
        : m_type(type), m_placement(std::move(placement)), m_displayName(std::move(displayName)) {}

    QString name() const override { return QStringLiteral("CreateBlock"); }
    CommandResult apply(const DesignModel::DesignDocument& input) const override;

private:
    DesignModel::BlockType m_type{DesignModel::BlockType::Unknown};
    DesignModel::Placement m_placement{};
    QString m_displayName;
};

class CreatePortCommand final : public DesignCommand {
public:
    CreatePortCommand(DesignModel::BlockId owner,
                      DesignModel::PortDirection dir,
                      DesignModel::PortType type,
                      QString name = {},
                      int capacity = 1)
        : m_owner(owner)
        , m_dir(dir)
        , m_type(std::move(type))
        , m_name(std::move(name))
        , m_capacity(capacity) {}

    QString name() const override { return QStringLiteral("CreatePort"); }
    CommandResult apply(const DesignModel::DesignDocument& input) const override;

private:
    DesignModel::BlockId m_owner{};
    DesignModel::PortDirection m_dir{DesignModel::PortDirection::Input};
    DesignModel::PortType m_type{};
    QString m_name;
    int m_capacity{1};
};

class CreateLinkCommand final : public DesignCommand {
public:
    CreateLinkCommand(DesignModel::PortId from, DesignModel::PortId to, QString label = {})
        : m_from(from), m_to(to), m_label(std::move(label)) {}

    QString name() const override { return QStringLiteral("CreateLink"); }
    CommandResult apply(const DesignModel::DesignDocument& input) const override;

private:
    DesignModel::PortId m_from{};
    DesignModel::PortId m_to{};
    QString m_label;
};

class AdjustLinkRouteCommand final : public DesignCommand {
public:
    AdjustLinkRouteCommand(DesignModel::LinkId linkId,
                           std::optional<DesignModel::RouteOverride> oldOverride,
                           std::optional<DesignModel::RouteOverride> newOverride)
        : m_linkId(linkId)
        , m_oldOverride(std::move(oldOverride))
        , m_newOverride(std::move(newOverride)) {}

    QString name() const override { return QStringLiteral("AdjustLinkRoute"); }
    CommandResult apply(const DesignModel::DesignDocument& input) const override;

private:
    DesignModel::LinkId m_linkId{};
    std::optional<DesignModel::RouteOverride> m_oldOverride;
    std::optional<DesignModel::RouteOverride> m_newOverride;
};

} // namespace Command

Q_DECLARE_METATYPE(Command::CreatedBlock)
Q_DECLARE_METATYPE(Command::CreatedPort)
Q_DECLARE_METATYPE(Command::CreatedLink)
