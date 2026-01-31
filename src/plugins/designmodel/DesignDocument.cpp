#include "designmodel/DesignDocument.hpp"

#include "utils/Macros.hpp"

#include <algorithm>

namespace DesignModel {

bool DesignDocument::Data::isValid() const noexcept {
    if (!version.isValid() || !metadata.isValid())
        return false;

    for (const auto& bid : blockOrder) {
        const auto bit = blocks.constFind(bid);
        if (bit == blocks.constEnd()) return false;

        for (const auto& pid : bit->ports()) {
            const auto pit = ports.constFind(pid);
            if (pit == ports.constEnd()) return false;
            if (pit->owner() != bid) return false;
        }
    }

    for (const auto& pid : portOrder) {
        const auto pit = ports.constFind(pid);
        if (pit == ports.constEnd()) return false;

        const BlockId owner = pit->owner();
        if (blocks.constFind(owner) == blocks.constEnd()) return false;
    }

    for (const auto& lid : linkOrder) {
        const auto lit = links.constFind(lid);
        if (lit == links.constEnd()) return false;

        if (ports.constFind(lit->from()) == ports.constEnd()) return false;
        if (ports.constFind(lit->to())   == ports.constEnd()) return false;
    }

    for (const auto& nid : netOrder) {
        const auto nit = nets.constFind(nid);
        if (nit == nets.constEnd()) return false;

        for (const auto& lid : nit->links()) {
            if (links.constFind(lid) == links.constEnd()) return false;
        }
    }

    for (const auto& id : annotationOrder) {
        const auto it = annotations.constFind(id);
        if (it == annotations.constEnd() || !it->isValid())
            return false;
    }

    for (const auto& rid : routeOrder) {
        const auto rit = routes.constFind(rid);
        if (rit == routes.constEnd()) return false;

        if (links.constFind(rit->link()) == links.constEnd()) return false;
    }

    return true;
}

DesignDocument::DesignDocument()
    : d(nullptr) {}

DesignDocument::~DesignDocument() = default;

DesignDocument::DesignDocument(QExplicitlySharedDataPointer<Data> data)
    : d(std::move(data)) {}

const DesignSchemaVersion& DesignDocument::schemaVersion() const noexcept {
    static const DesignSchemaVersion kInvalid = DesignSchemaVersion::invalid();
    return d ? d->version : kInvalid;
}

const DesignMetadata& DesignDocument::metadata() const noexcept {
    static const DesignMetadata kEmpty{};
    return d ? d->metadata : kEmpty;
}

const DesignIndex& DesignDocument::index() const noexcept {
    static const DesignIndex kEmpty{};
    return d ? d->index : kEmpty;
}

const QVector<BlockId>& DesignDocument::blockIds() const noexcept {
    static const QVector<BlockId> kEmpty{};
    return d ? d->blockOrder : kEmpty;
}

const QVector<PortId>& DesignDocument::portIds() const noexcept {
    static const QVector<PortId> kEmpty{};
    return d ? d->portOrder : kEmpty;
}

const QVector<LinkId>& DesignDocument::linkIds() const noexcept {
    static const QVector<LinkId> kEmpty{};
    return d ? d->linkOrder : kEmpty;
}

const QVector<NetId>& DesignDocument::netIds() const noexcept {
    static const QVector<NetId> kEmpty{};
    return d ? d->netOrder : kEmpty;
}

const QVector<AnnotationId>& DesignDocument::annotationIds() const noexcept {
    static const QVector<AnnotationId> kEmpty{};
    return d ? d->annotationOrder : kEmpty;
}

const QVector<RouteId>& DesignDocument::routeIds() const noexcept {
    static const QVector<RouteId> kEmpty{};
    return d ? d->routeOrder : kEmpty;
}

const Block* DesignDocument::tryBlock(BlockId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->blocks.constFind(id);
    return it == d->blocks.constEnd() ? nullptr : &(*it);
}

const Port* DesignDocument::tryPort(PortId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->ports.constFind(id);
    return it == d->ports.constEnd() ? nullptr : &(*it);
}

const Link* DesignDocument::tryLink(LinkId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->links.constFind(id);
    return it == d->links.constEnd() ? nullptr : &(*it);
}

const Net* DesignDocument::tryNet(NetId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->nets.constFind(id);
    return it == d->nets.constEnd() ? nullptr : &(*it);
}

const Annotation* DesignDocument::tryAnnotation(AnnotationId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->annotations.constFind(id);
    return it == d->annotations.constEnd() ? nullptr : &(*it);
}

const Route* DesignDocument::tryRoute(RouteId id) const noexcept {
    if (!d) return nullptr;
    const auto it = d->routes.constFind(id);
    return it == d->routes.constEnd() ? nullptr : &(*it);
}

bool DesignDocument::isValid() const noexcept {
    return d && d->isValid();
}

DesignDocument::Builder::Builder(DesignSchemaVersion v, DesignMetadata md)
    : m_version(v)
    , m_metadata(std::move(md)) {}

DesignDocument::Builder::Builder(const DesignDocument& doc)
    : m_version(doc.schemaVersion())
    , m_metadata(doc.metadata())
{
    for (const BlockId id : doc.blockIds()) {
        const Block* b = doc.tryBlock(id);
        if (!b) continue;
        m_blocks.insert(id, *b);
        m_blockOrder.push_back(id);
    }

    for (const PortId id : doc.portIds()) {
        const Port* p = doc.tryPort(id);
        if (!p) continue;
        m_ports.insert(id, *p);
        m_portOrder.push_back(id);
    }

    for (const LinkId id : doc.linkIds()) {
        const Link* l = doc.tryLink(id);
        if (!l) continue;
        m_links.insert(id, *l);
        m_linkOrder.push_back(id);
    }

    for (const NetId id : doc.netIds()) {
        const Net* n = doc.tryNet(id);
        if (!n) continue;
        m_nets.insert(id, *n);
        m_netOrder.push_back(id);
    }

    for (const AnnotationId id : doc.annotationIds()) {
        const Annotation* a = doc.tryAnnotation(id);
        if (!a) continue;
        m_annotations.insert(id, *a);
        m_annotationOrder.push_back(id);
    }

    for (const RouteId id : doc.routeIds()) {
        const Route* r = doc.tryRoute(id);
        if (!r) continue;
        m_routes.insert(id, *r);
        m_routeOrder.push_back(id);
    }
}

BlockId DesignDocument::Builder::createBlock(BlockType type, Placement placement, QString displayName) {
    const BlockId id = BlockId::create();
    Block b(id, type, std::move(placement), std::move(displayName));
    m_blocks.insert(id, b);
    m_blockOrder.push_back(id);
    return id;
}

PortId DesignDocument::Builder::createPort(BlockId owner,
                                          PortDirection dir,
                                          PortType type,
                                          QString name,
                                          int capacity) {
    const PortId id = PortId::create();
    Port p(id, owner, dir, std::move(type), std::move(name), capacity);
    m_ports.insert(id, p);
    m_portOrder.push_back(id);

    if (auto it = m_blocks.find(owner); it != m_blocks.end())
        it->addPort(id);

    return id;
}

LinkId DesignDocument::Builder::createLink(PortId from, PortId to, QString label) {
    const LinkId id = LinkId::create();
    Link l(id, from, to, std::move(label));
    m_links.insert(id, l);
    m_linkOrder.push_back(id);
    return id;
}

bool DesignDocument::Builder::setLinkRouteOverride(LinkId id, std::optional<RouteOverride> routeOverride) {
    auto it = m_links.find(id);
    UTILS_GUARD_RET(it != m_links.end(), false);

    const auto& existing = it.value();
    if (existing.routeOverride() == routeOverride)
        return false;

    it.value() = Link(existing.id(), existing.from(), existing.to(), existing.label(), std::move(routeOverride));
    return true;
}

NetId DesignDocument::Builder::createNet(QString name, QVector<LinkId> links) {
    const NetId id = NetId::create();
    Net n(id, std::move(name), std::move(links));
    m_nets.insert(id, n);
    m_netOrder.push_back(id);
    return id;
}

AnnotationId DesignDocument::Builder::createAnnotation(AnnotationKind kind,
                                                      QString text,
                                                      QVector<BlockId> blocks,
                                                      QVector<PortId> ports,
                                                      QVector<LinkId> links,
                                                      QVector<TileCoord> tiles,
                                                      QString tag) {
    const AnnotationId id = AnnotationId::create();
    Annotation a(id, kind, std::move(text),
                 std::move(blocks), std::move(ports), std::move(links), std::move(tiles),
                 std::move(tag));
    m_annotations.insert(id, a);
    m_annotationOrder.push_back(id);
    return id;
}

RouteId DesignDocument::Builder::createRoute(LinkId link, QVector<TileCoord> path) {
    const RouteId id = RouteId::create();
    Route r(id, link, std::move(path));
    m_routes.insert(id, r);
    m_routeOrder.push_back(id);
    return id;
}

bool DesignDocument::Builder::removeRoute(RouteId id) {
    if (id.isNull())
        return false;
    if (!m_routes.remove(id))
        return false;
    m_routeOrder.erase(std::remove(m_routeOrder.begin(), m_routeOrder.end(), id), m_routeOrder.end());
    return true;
}

bool DesignDocument::Builder::removeLink(LinkId id) {
    if (id.isNull())
        return false;
    if (!m_links.remove(id))
        return false;

    m_linkOrder.erase(std::remove(m_linkOrder.begin(), m_linkOrder.end(), id), m_linkOrder.end());

    for (auto it = m_nets.begin(); it != m_nets.end(); ++it) {
        auto links = it->links();
        const int before = links.size();
        links.erase(std::remove(links.begin(), links.end(), id), links.end());
        if (links.size() != before)
            *it = Net(it.key(), it->name(), links);
    }

    QVector<RouteId> toRemove;
    toRemove.reserve(m_routes.size());
    for (auto it = m_routes.constBegin(); it != m_routes.constEnd(); ++it) {
        if (it->link() == id)
            toRemove.push_back(it.key());
    }
    for (const auto rid : toRemove)
        removeRoute(rid);

    for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
        auto linkTargets = it->linkTargets();
        const int before = linkTargets.size();
        linkTargets.erase(std::remove(linkTargets.begin(), linkTargets.end(), id), linkTargets.end());
        if (linkTargets.size() != before) {
            *it = Annotation(it.key(), it->kind(), it->text(),
                             it->blockTargets(), it->portTargets(), linkTargets, it->tileTargets(), it->tag());
        }
    }

    return true;
}

static void erasePortFromBlockPorts(QHash<BlockId, Block>& blocks, BlockId owner, PortId pid)
{
    auto it = blocks.find(owner);
    if (it == blocks.end())
        return;

    QVector<PortId> ports = it->ports();
    ports.erase(std::remove(ports.begin(), ports.end(), pid), ports.end());

    Block b(it.key(), it->type(), it->placement(), it->displayName());
    for (const auto p : ports)
        b.addPort(p);
    it.value() = b;
}

static QVector<LinkId> linksTouchingPort(const QHash<LinkId, Link>& links, PortId pid)
{
    QVector<LinkId> out;
    out.reserve(8);
    for (auto it = links.constBegin(); it != links.constEnd(); ++it) {
        if (it->from() == pid || it->to() == pid)
            out.push_back(it.key());
    }
    return out;
}

bool DesignDocument::Builder::removeBlock(BlockId id) {
    if (id.isNull())
        return false;
    auto bit = m_blocks.find(id);
    if (bit == m_blocks.end())
        return false;

    const QVector<PortId> ports = bit->ports();

    QVector<LinkId> linkIds;
    for (const auto pid : ports) {
        const auto touched = linksTouchingPort(m_links, pid);
        linkIds += touched;
    }
    std::sort(linkIds.begin(), linkIds.end());
    linkIds.erase(std::unique(linkIds.begin(), linkIds.end()), linkIds.end());
    for (const auto lid : linkIds)
        removeLink(lid);

    for (const auto pid : ports) {
        m_ports.remove(pid);
        m_portOrder.erase(std::remove(m_portOrder.begin(), m_portOrder.end(), pid), m_portOrder.end());
        erasePortFromBlockPorts(m_blocks, id, pid);

        for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
            auto portTargets = it->portTargets();
            const int before = portTargets.size();
            portTargets.erase(std::remove(portTargets.begin(), portTargets.end(), pid), portTargets.end());
            if (portTargets.size() != before) {
                *it = Annotation(it.key(), it->kind(), it->text(),
                                 it->blockTargets(), portTargets, it->linkTargets(), it->tileTargets(), it->tag());
            }
        }
    }

    for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
        auto blockTargets = it->blockTargets();
        const int before = blockTargets.size();
        blockTargets.erase(std::remove(blockTargets.begin(), blockTargets.end(), id), blockTargets.end());
        if (blockTargets.size() != before) {
            *it = Annotation(it.key(), it->kind(), it->text(),
                             blockTargets, it->portTargets(), it->linkTargets(), it->tileTargets(), it->tag());
        }
    }

    m_blocks.erase(bit);
    m_blockOrder.erase(std::remove(m_blockOrder.begin(), m_blockOrder.end(), id), m_blockOrder.end());

    return true;
}

bool DesignDocument::Builder::removeAnnotation(AnnotationId id) {
    if (id.isNull())
        return false;
    if (!m_annotations.remove(id))
        return false;
    m_annotationOrder.erase(std::remove(m_annotationOrder.begin(), m_annotationOrder.end(), id), m_annotationOrder.end());
    return true;
}

bool DesignDocument::Builder::removeNet(NetId id) {
    if (id.isNull())
        return false;
    if (!m_nets.remove(id))
        return false;
    m_netOrder.erase(std::remove(m_netOrder.begin(), m_netOrder.end(), id), m_netOrder.end());
    return true;
}

DesignDocument DesignDocument::Builder::freeze() const {
    QExplicitlySharedDataPointer<Data> data(new Data());
    data->version  = m_version;
    data->metadata = m_metadata;

    data->blocks = m_blocks;
    data->ports  = m_ports;
    data->links  = m_links;

    data->nets        = m_nets;
    data->annotations = m_annotations;
    data->routes      = m_routes;

    data->blockOrder = m_blockOrder;
    data->portOrder  = m_portOrder;
    data->linkOrder  = m_linkOrder;

    data->netOrder        = m_netOrder;
    data->annotationOrder = m_annotationOrder;
    data->routeOrder      = m_routeOrder;

    data->index = DesignIndex(data->blockOrder, data->linkOrder,
                              data->blocks, data->ports, data->links);

    return DesignDocument(std::move(data));
}

} // namespace DesignModel