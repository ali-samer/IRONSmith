// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/hlir_sync/DesignVerifier.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"

#include <QtCore/QHash>
#include <QtCore/QSet>

#include <optional>

namespace Aie::Internal {

// ---------------------------------------------------------------------------
// Local helpers — node taxonomy and wire topology
// ---------------------------------------------------------------------------

namespace {

/// Local node kind — includes DDR which is not an AIE tile but is a canvas block.
enum class NodeKind { SHIM, MEM, COMPUTE, DDR };

/// Parsed node kind and grid position extracted from a specId.
struct ParsedSpec {
    NodeKind kind;
    int col = 0;
    int row = 0;
};

/// Parse a specId ("shim0_0", "mem1_2", "aie0_3", "ddr") into kind and coordinates.
std::optional<ParsedSpec> parseTileSpec(const QString& specId)
{
    if (specId == QLatin1StringView("ddr"))
        return ParsedSpec{NodeKind::DDR, 0, 0};

    static const struct { const char* prefix; NodeKind kind; } prefixes[] = {
        { "shim", NodeKind::SHIM    },
        { "mem",  NodeKind::MEM     },
        { "aie",  NodeKind::COMPUTE },
    };

    for (const auto& p : prefixes) {
        const QLatin1StringView prefix{p.prefix};
        if (!specId.startsWith(prefix))
            continue;

        const QString rest = specId.sliced(prefix.size());
        const qsizetype underIdx = rest.indexOf(u'_');
        if (underIdx < 0)
            continue;

        bool okCol = false, okRow = false;
        const int col = rest.left(underIdx).toInt(&okCol);
        const int row = rest.sliced(underIdx + 1).toInt(&okRow);
        if (!okCol || !okRow)
            continue;

        return ParsedSpec{p.kind, col, row};
    }
    return std::nullopt;
}

/// A wire whose both endpoints resolve to known canvas nodes (tiles or DDR).
struct ParsedWire {
    Canvas::CanvasWire*  wire          = nullptr;
    Canvas::CanvasBlock* producerBlock = nullptr; // endpoint A
    Canvas::CanvasBlock* consumerBlock = nullptr; // endpoint B
    ParsedSpec           producerSpec{};
    ParsedSpec           consumerSpec{};
    QString              fifoName;

    // Fill: DDR drives a SHIM — data flows DDR → SHIM → array.
    bool isFill() const
    {
        return producerSpec.kind == NodeKind::DDR && consumerSpec.kind == NodeKind::SHIM;
    }

    // Drain: a SHIM delivers to DDR — data flows array → SHIM → DDR.
    bool isDrain() const
    {
        return producerSpec.kind == NodeKind::SHIM && consumerSpec.kind == NodeKind::DDR;
    }

    // Object FIFO: a data-path wire between two non-DDR tiles.
    bool isObjectFifo() const
    {
        return producerSpec.kind != NodeKind::DDR && consumerSpec.kind != NodeKind::DDR;
    }
};

/// Format a node as "Shim(0,0)", "Mem(1,2)", "AIE(0,4)", or "DDR" for use in messages.
QString tileName(const ParsedSpec& spec)
{
    switch (spec.kind) {
        case NodeKind::SHIM:    return QStringLiteral("Shim(%1,%2)").arg(spec.col).arg(spec.row);
        case NodeKind::MEM:     return QStringLiteral("Mem(%1,%2)").arg(spec.col).arg(spec.row);
        case NodeKind::COMPUTE: return QStringLiteral("AIE(%1,%2)").arg(spec.col).arg(spec.row);
        case NodeKind::DDR:     return QStringLiteral("DDR");
    }
    return QStringLiteral("Tile");
}

/// Collect all fully-attached wires whose both endpoints have known specIds (tiles or DDR).
QList<ParsedWire> collectWires(const Canvas::CanvasDocument& doc)
{
    QList<ParsedWire> result;

    for (const auto& item : doc.items()) {
        auto* wire = dynamic_cast<Canvas::CanvasWire*>(item.get());
        if (!wire)
            continue;

        const auto& epA = wire->a();
        const auto& epB = wire->b();
        if (!epA.attached.has_value() || !epB.attached.has_value())
            continue;

        auto* blockA = dynamic_cast<Canvas::CanvasBlock*>(doc.findItem(epA.attached->itemId));
        auto* blockB = dynamic_cast<Canvas::CanvasBlock*>(doc.findItem(epB.attached->itemId));
        if (!blockA || !blockB)
            continue;

        const auto parsedA = parseTileSpec(blockA->specId());
        const auto parsedB = parseTileSpec(blockB->specId());
        if (!parsedA || !parsedB)
            continue;

        QString fifoName;
        if (wire->hasObjectFifo())
            fifoName = wire->objectFifo().value().name;
        else
            fifoName = QStringLiteral("fifo_%1_to_%2").arg(blockA->specId(), blockB->specId());

        result.append(ParsedWire{wire, blockA, blockB, *parsedA, *parsedB, fifoName});
    }

    return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Check 1 — RuntimeSequenceDefined
//
// A valid design needs at least one Fill (DDR → SHIM) and at least one
// Drain (SHIM → DDR) to form a complete runtime I/O sequence.
// ---------------------------------------------------------------------------

class RuntimeSequenceCheck : public IVerificationCheck
{
public:
    QString name() const override { return QStringLiteral("RuntimeSequenceDefined"); }

    QList<VerificationIssue> run(const VerificationContext& ctx) const override
    {
        QList<VerificationIssue> issues;
        if (!ctx.document)
            return issues;

        int fillCount = 0;
        int drainCount = 0;
        for (const auto& w : collectWires(*ctx.document)) {
            if (w.isFill())  ++fillCount;
            if (w.isDrain()) ++drainCount;
        }

        if (fillCount == 0)
            issues.append({VerificationIssue::Severity::Error,
                QStringLiteral("No Fill defined — connect DDR to a SHIM tile "
                               "(DDR \u2192 SHIM \u2192 array).")});

        if (drainCount == 0)
            issues.append({VerificationIssue::Severity::Error,
                QStringLiteral("No Drain defined — connect a SHIM tile to DDR "
                               "(array \u2192 SHIM \u2192 DDR).")});

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 2 — ShimFillConnectivity
//
// Each SHIM tile that receives a Fill (DDR → SHIM) must also have at least
// one object FIFO going OUT into the array (SHIM → MEM or AIE). Without it
// the incoming DDR data has nowhere to go.
// ---------------------------------------------------------------------------

class ShimFillCheck : public IVerificationCheck
{
public:
    QString name() const override { return QStringLiteral("ShimFillConnectivity"); }

    QList<VerificationIssue> run(const VerificationContext& ctx) const override
    {
        QList<VerificationIssue> issues;
        if (!ctx.document)
            return issues;

        const auto wires = collectWires(*ctx.document);

        // SHIM blocks that produce at least one outgoing object FIFO (SHIM → MEM/AIE).
        QSet<Canvas::CanvasBlock*> shimsWithOutgoingFifo;
        for (const auto& w : wires) {
            if (w.isObjectFifo() && w.producerSpec.kind == NodeKind::SHIM)
                shimsWithOutgoingFifo.insert(w.producerBlock);
        }

        // Every fill SHIM must have an outgoing FIFO.
        for (const auto& w : wires) {
            if (!w.isFill())
                continue;
            // consumerBlock is the SHIM tile in a DDR → SHIM wire.
            if (!shimsWithOutgoingFifo.contains(w.consumerBlock)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral("Fill at %1: no FIFO leads from this SHIM into the array.")
                    .arg(tileName(w.consumerSpec))});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 3 — ShimDrainConnectivity
//
// Each SHIM tile that feeds a Drain (SHIM → DDR) must also have at least
// one object FIFO coming IN from the array (MEM or AIE → SHIM). Without it
// there is no array-computed data to drain.
// ---------------------------------------------------------------------------

class ShimDrainCheck : public IVerificationCheck
{
public:
    QString name() const override { return QStringLiteral("ShimDrainConnectivity"); }

    QList<VerificationIssue> run(const VerificationContext& ctx) const override
    {
        QList<VerificationIssue> issues;
        if (!ctx.document)
            return issues;

        const auto wires = collectWires(*ctx.document);

        // SHIM blocks that consume at least one incoming object FIFO (MEM/AIE → SHIM).
        QSet<Canvas::CanvasBlock*> shimsWithIncomingFifo;
        for (const auto& w : wires) {
            if (w.isObjectFifo() && w.consumerSpec.kind == NodeKind::SHIM)
                shimsWithIncomingFifo.insert(w.consumerBlock);
        }

        // Every drain SHIM must have an incoming FIFO.
        for (const auto& w : wires) {
            if (!w.isDrain())
                continue;
            // producerBlock is the SHIM tile in a SHIM → DDR wire.
            if (!shimsWithIncomingFifo.contains(w.producerBlock)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral("Drain at %1: no FIFO leads into this SHIM from the array.")
                    .arg(tileName(w.producerSpec))});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 4 — DisconnectedDataflow
//
// Every MEM/AIE tile that participates in the dataflow must have FIFO
// connections on BOTH sides — at least one incoming and at least one
// outgoing. SHIM tiles are excluded because they are intentional endpoints
// (one side connects to DDR, the other to the array via FIFO). DDR is
// excluded because it is the external memory boundary.
// ---------------------------------------------------------------------------

class DisconnectedDataflowCheck : public IVerificationCheck
{
public:
    QString name() const override { return QStringLiteral("DisconnectedDataflow"); }

    QList<VerificationIssue> run(const VerificationContext& ctx) const override
    {
        QList<VerificationIssue> issues;
        if (!ctx.document)
            return issues;

        // Count in/out object FIFO connections for MEM/AIE tiles only.
        QHash<Canvas::CanvasBlock*, ParsedSpec> blockSpec;
        QHash<Canvas::CanvasBlock*, int> inCount;
        QHash<Canvas::CanvasBlock*, int> outCount;

        for (const auto& w : collectWires(*ctx.document)) {
            if (!w.isObjectFifo())
                continue; // DDR wires are handled by ShimFill/DrainCheck

            if (w.producerSpec.kind != NodeKind::SHIM) {
                blockSpec.insert(w.producerBlock, w.producerSpec);
                outCount[w.producerBlock]++;
            }
            if (w.consumerSpec.kind != NodeKind::SHIM) {
                blockSpec.insert(w.consumerBlock, w.consumerSpec);
                inCount[w.consumerBlock]++;
            }
        }

        // Tile has outgoing FIFOs but no incoming — data appears from nowhere.
        for (auto it = outCount.cbegin(); it != outCount.cend(); ++it) {
            Canvas::CanvasBlock* block = it.key();
            if (!inCount.contains(block)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral("%1 has no incoming FIFO — no upstream data source.")
                    .arg(tileName(blockSpec[block]))});
            }
        }

        // Tile has incoming FIFOs but no outgoing — data flows in and goes nowhere.
        for (auto it = inCount.cbegin(); it != inCount.cend(); ++it) {
            Canvas::CanvasBlock* block = it.key();
            if (!outCount.contains(block)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral("%1 has no outgoing FIFO — data has no downstream path.")
                    .arg(tileName(blockSpec[block]))});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 5 — DmaChannelLimit
//
// Each tile type has a fixed number of DMA channels. All connections
// (both DDR wires and object FIFO wires) count toward a tile's channel
// budget. DDR itself is excluded — it is not a real tile with a channel limit.
//   SHIM   — 4 channels  (error at 5 or more)
//   MEM    — 6 channels  (error at 7 or more)
//   AIE    — 4 channels  (error at 5 or more)
// ---------------------------------------------------------------------------

class DmaChannelLimitCheck : public IVerificationCheck
{
    static int channelLimit(NodeKind kind)
    {
        switch (kind) {
            case NodeKind::SHIM:    return 4;
            case NodeKind::MEM:     return 6;
            case NodeKind::COMPUTE: return 4;
            case NodeKind::DDR:     return INT_MAX;
        }
        return 4;
    }

public:
    QString name() const override { return QStringLiteral("DmaChannelLimit"); }

    QList<VerificationIssue> run(const VerificationContext& ctx) const override
    {
        QList<VerificationIssue> issues;
        if (!ctx.document)
            return issues;

        // Count all connections (DDR wires + FIFO wires) for every non-DDR tile.
        QHash<Canvas::CanvasBlock*, ParsedSpec> blockSpec;
        QHash<Canvas::CanvasBlock*, int> connectionCount;

        for (const auto& w : collectWires(*ctx.document)) {
            if (w.producerSpec.kind != NodeKind::DDR) {
                blockSpec.insert(w.producerBlock, w.producerSpec);
                connectionCount[w.producerBlock]++;
            }
            if (w.consumerSpec.kind != NodeKind::DDR) {
                blockSpec.insert(w.consumerBlock, w.consumerSpec);
                connectionCount[w.consumerBlock]++;
            }
        }

        for (auto it = connectionCount.cbegin(); it != connectionCount.cend(); ++it) {
            const ParsedSpec& spec = blockSpec[it.key()];
            const int limit = channelLimit(spec.kind);
            if (it.value() > limit) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral("%1 has %2 connections but only %3 DMA channels are available.")
                    .arg(tileName(spec)).arg(it.value()).arg(limit)});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// DesignVerifier
// ---------------------------------------------------------------------------

DesignVerifier::DesignVerifier()
{
    // Register checks in the order they will be reported to the user.
    m_checks.push_back(std::make_unique<RuntimeSequenceCheck>());
    m_checks.push_back(std::make_unique<ShimFillCheck>());
    m_checks.push_back(std::make_unique<ShimDrainCheck>());
    m_checks.push_back(std::make_unique<DisconnectedDataflowCheck>());
    m_checks.push_back(std::make_unique<DmaChannelLimitCheck>());
}

DesignVerifier::~DesignVerifier() = default;

QList<VerificationIssue> DesignVerifier::verify(const VerificationContext& ctx) const
{
    // Run every registered check and collect all issues.
    QList<VerificationIssue> all;
    for (const auto& check : m_checks)
        all.append(check->run(ctx));
    return all;
}

bool DesignVerifier::hasErrors(const QList<VerificationIssue>& issues)
{
    for (const auto& issue : issues) {
        if (issue.severity == VerificationIssue::Severity::Error)
            return true;
    }
    return false;
}

DesignStats collectStats(const VerificationContext& ctx)
{
    DesignStats stats;
    if (!ctx.document)
        return stats;

    // Only count non-DDR tiles that participate in at least one connection.
    QHash<Canvas::CanvasBlock*, ParsedSpec> connectedTiles;

    for (const auto& w : collectWires(*ctx.document)) {
        if (w.isObjectFifo()) ++stats.fifos;
        if (w.isFill())       ++stats.fills;
        if (w.isDrain())      ++stats.drains;

        if (w.producerSpec.kind != NodeKind::DDR)
            connectedTiles.insert(w.producerBlock, w.producerSpec);
        if (w.consumerSpec.kind != NodeKind::DDR)
            connectedTiles.insert(w.consumerBlock, w.consumerSpec);
    }

    for (auto it = connectedTiles.cbegin(); it != connectedTiles.cend(); ++it) {
        switch (it.value().kind) {
            case NodeKind::SHIM:    ++stats.shimTiles; break;
            case NodeKind::MEM:     ++stats.memTiles;  break;
            case NodeKind::COMPUTE: ++stats.aieTiles;  break;
            case NodeKind::DDR:     break;
        }
    }

    return stats;
}

} // namespace Aie::Internal
