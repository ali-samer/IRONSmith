// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/hlir_sync/DesignVerifier.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"
#include "hlir_cpp_bridge/HlirTypes.hpp"

#include <QtCore/QHash>

#include <optional>

namespace Aie::Internal {

// ---------------------------------------------------------------------------
// Local helpers — wire topology analysis
// ---------------------------------------------------------------------------

namespace {

/// Parsed tile kind and grid position extracted from a specId.
struct ParsedSpec {
    hlir::TileKind kind;
    int col = 0;
    int row = 0;
};

/// Parse a tile specId ("shim0_0", "mem1_2", "aie0_3") into kind and coordinates.
std::optional<ParsedSpec> parseTileSpec(const QString& specId)
{
    static const struct { const char* prefix; hlir::TileKind kind; } prefixes[] = {
        { "shim", hlir::TileKind::SHIM    },
        { "mem",  hlir::TileKind::MEM     },
        { "aie",  hlir::TileKind::COMPUTE },
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

/// A wire whose both endpoints resolve to tiles with valid specIds.
struct ParsedWire {
    Canvas::CanvasWire*  wire          = nullptr;
    Canvas::CanvasBlock* producerBlock = nullptr; // endpoint A (data source in this FIFO)
    Canvas::CanvasBlock* consumerBlock = nullptr; // endpoint B (data sink in this FIFO)
    ParsedSpec           producerSpec{};
    ParsedSpec           consumerSpec{};
    QString              fifoName;

    // A wire is a Fill when the SHIM tile is the FIFO producer —
    // the SHIM serves as the DDR input gateway (DDR → SHIM → array tile).
    bool isFill()  const { return producerSpec.kind == hlir::TileKind::SHIM; }

    // A wire is a Drain when the SHIM tile is the FIFO consumer —
    // the SHIM serves as the DDR output gateway (array tile → SHIM → DDR).
    bool isDrain() const { return consumerSpec.kind == hlir::TileKind::SHIM; }
};

/// Collect all fully-attached wires whose both endpoints have valid specIds.
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
// A valid design needs at least one Fill (DDR → SHIM → array) and at least one
// Drain (array → SHIM → DDR) to form a complete runtime I/O sequence.
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
                QStringLiteral(
                    "No Fill operations defined: the design has no DDR input path. "
                    "At least one FIFO must have a SHIM tile as its producer — "
                    "place a SHIM tile and connect it as the source of an object FIFO "
                    "going into the device array (DDR \u2192 SHIM \u2192 array).")});

        if (drainCount == 0)
            issues.append({VerificationIssue::Severity::Error,
                QStringLiteral(
                    "No Drain operations defined: the design has no DDR output path. "
                    "At least one FIFO must have a SHIM tile as its consumer — "
                    "place a SHIM tile and connect it as the destination of an object FIFO "
                    "coming from the device array (array \u2192 SHIM \u2192 DDR).")});

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 2 — ShimFillConnectivity
//
// A Fill brings data from DDR into the device through a SHIM tile. The SHIM
// therefore needs a valid object FIFO going OUT into the array (to a MEM or
// AIE tile). Connecting a fill SHIM directly to another SHIM tile means the
// incoming DDR data has nowhere to go in the array.
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

        for (const auto& w : collectWires(*ctx.document)) {
            if (!w.isFill())
                continue;

            // The fill SHIM is the producer (endpoint A). The array-side tile is the consumer
            // (endpoint B). Verify that endpoint B is a MEM or AIE tile, not another SHIM.
            if (w.consumerSpec.kind == hlir::TileKind::SHIM) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral(
                        "Invalid Fill connection on SHIM tile '%1': "
                        "the outgoing object FIFO '%2' connects directly to another SHIM tile '%3'. "
                        "A Fill delivers DDR data from SHIM '%1' into the device array — "
                        "the outgoing FIFO must connect to a MEM or AIE tile, not another SHIM.")
                    .arg(w.producerBlock->specId(), w.fifoName, w.consumerBlock->specId())});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 3 — ShimDrainConnectivity
//
// A Drain collects results from the device array and transfers them to DDR
// through a SHIM tile. The SHIM therefore needs a valid object FIFO coming IN
// from the array (from a MEM or AIE tile). A drain SHIM connected directly to
// another SHIM means there is no array-computed data to drain.
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

        for (const auto& w : collectWires(*ctx.document)) {
            if (!w.isDrain())
                continue;

            // The drain SHIM is the consumer (endpoint B). The array-side tile is the producer
            // (endpoint A). Verify that endpoint A is a MEM or AIE tile, not another SHIM.
            if (w.producerSpec.kind == hlir::TileKind::SHIM) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral(
                        "Invalid Drain connection on SHIM tile '%1': "
                        "the incoming object FIFO '%2' comes directly from another SHIM tile '%3'. "
                        "A Drain collects data from the device array for SHIM '%1' to transfer to DDR — "
                        "the incoming FIFO must come from a MEM or AIE tile, not another SHIM.")
                    .arg(w.consumerBlock->specId(), w.fifoName, w.producerBlock->specId())});
            }
        }

        return issues;
    }
};

// ---------------------------------------------------------------------------
// Check 4 — DisconnectedDataflow
//
// Every non-SHIM tile that participates in the dataflow must have FIFO
// connections on BOTH sides — at least one incoming FIFO (data in) and at
// least one outgoing FIFO (data out). A tile with only inputs creates a dead
// end; a tile with only outputs has no data source. SHIM tiles are excluded
// because they are intentionally one-directional (fill = output only,
// drain = input only).
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

        // Count each non-SHIM tile's incoming and outgoing FIFO connections.
        // SHIM tiles are skipped — they are the intentional endpoints of the flow.
        QHash<Canvas::CanvasBlock*, int> inCount;
        QHash<Canvas::CanvasBlock*, int> outCount;

        for (const auto& w : collectWires(*ctx.document)) {
            if (w.producerSpec.kind != hlir::TileKind::SHIM)
                outCount[w.producerBlock]++;
            if (w.consumerSpec.kind != hlir::TileKind::SHIM)
                inCount[w.consumerBlock]++;
        }

        // Tile has outgoing FIFOs but no incoming FIFOs — data appears from nowhere
        for (auto it = outCount.cbegin(); it != outCount.cend(); ++it) {
            Canvas::CanvasBlock* block = it.key();
            if (!inCount.contains(block)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral(
                        "Disconnected dataflow: tile '%1' has outgoing FIFOs but no incoming FIFO. "
                        "This tile has no upstream data source — "
                        "connect an incoming object FIFO to '%1' or remove it from the design.")
                    .arg(block->specId())});
            }
        }

        // Tile has incoming FIFOs but no outgoing FIFOs — data flows in and goes nowhere
        for (auto it = inCount.cbegin(); it != inCount.cend(); ++it) {
            Canvas::CanvasBlock* block = it.key();
            if (!outCount.contains(block)) {
                issues.append({VerificationIssue::Severity::Error,
                    QStringLiteral(
                        "Disconnected dataflow: tile '%1' has incoming FIFOs but no outgoing FIFO. "
                        "Data flowing into this tile has no downstream path — "
                        "connect an outgoing object FIFO from '%1' or remove it from the design.")
                    .arg(block->specId())});
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

} // namespace Aie::Internal
