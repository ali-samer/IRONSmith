#include "command/CommandDispatcher.hpp"

#include <QtCore/QMetaType>

namespace Command {

CommandDispatcher::CommandDispatcher(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<Command::CommandResult>("Command::CommandResult");
}

void CommandDispatcher::setDocument(DesignModel::DesignDocument doc)
{
    m_doc = std::move(doc);
    m_undo.clear();
    m_redo.clear();

    emit documentChanged(m_doc);
    emitUndoRedoIfChanged();
}

void CommandDispatcher::emitUndoRedoIfChanged()
{
    emit undoRedoStateChanged(canUndo(), canRedo());
}

void CommandDispatcher::pushUndoSnapshotIfNeeded()
{
    if (!inTransaction()) {
        m_undo.push_back(m_doc);
        return;
    }

    if (!m_txTouched) {
        m_txBase = m_doc;
        m_txTouched = true;
    }
}

CommandResult CommandDispatcher::apply(const DesignCommand& command)
{
    const auto beforeCanUndo = canUndo();
    const auto beforeCanRedo = canRedo();

    const CommandResult r = command.apply(m_doc);

    emit commandApplied(command.name(), r);

    if (!r.ok())
        return r;

    pushUndoSnapshotIfNeeded();
    m_redo.clear();

    m_doc = r.document();
    emit documentChanged(m_doc);

    if (beforeCanUndo != canUndo() || beforeCanRedo != canRedo())
        emitUndoRedoIfChanged();

    return r;
}

CommandResult CommandDispatcher::undo()
{
    if (!canUndo())
        return CommandResult::failure(CommandError(CommandErrorCode::InvalidArgument, "Undo: nothing to undo."));
    if (inTransaction())
        return CommandResult::failure(CommandError(CommandErrorCode::InvalidArgument, "Undo: not allowed during transaction."));

    const auto beforeCanUndo = canUndo();
    const auto beforeCanRedo = canRedo();

    m_redo.push_back(m_doc);
    m_doc = m_undo.takeLast();

    emit documentChanged(m_doc);

    if (beforeCanUndo != canUndo() || beforeCanRedo != canRedo())
        emitUndoRedoIfChanged();

    return CommandResult::success(m_doc);
}

CommandResult CommandDispatcher::redo()
{
    if (!canRedo())
        return CommandResult::failure(CommandError(CommandErrorCode::InvalidArgument, "Redo: nothing to redo."));
    if (inTransaction())
        return CommandResult::failure(CommandError(CommandErrorCode::InvalidArgument, "Redo: not allowed during transaction."));

    const auto beforeCanUndo = canUndo();
    const auto beforeCanRedo = canRedo();

    m_undo.push_back(m_doc);
    m_doc = m_redo.takeLast();

    emit documentChanged(m_doc);

    if (beforeCanUndo != canUndo() || beforeCanRedo != canRedo())
        emitUndoRedoIfChanged();

    return CommandResult::success(m_doc);
}

void CommandDispatcher::beginTransaction(QString label)
{
    if (m_txDepth == 0) {
        m_txLabel = std::move(label);
        m_txTouched = false;
        m_txBase = DesignModel::DesignDocument{};
        emit transactionStateChanged(true, m_txLabel);
    }
    ++m_txDepth;
}

void CommandDispatcher::commitTransaction()
{
    if (m_txDepth <= 0)
        return;

    --m_txDepth;
    if (m_txDepth != 0)
        return;

    if (m_txTouched) {
        m_undo.push_back(m_txBase);
    }

    m_txLabel.clear();
    m_txTouched = false;
    m_txBase = DesignModel::DesignDocument{};

    emit transactionStateChanged(false, {});
    emitUndoRedoIfChanged();
}

void CommandDispatcher::rollbackTransaction()
{
    if (m_txDepth <= 0)
        return;

    if (m_txTouched) {
        m_doc = m_txBase;
        m_redo.clear();
        emit documentChanged(m_doc);
    }

    m_txDepth = 0;
    m_txLabel.clear();
    m_txTouched = false;
    m_txBase = DesignModel::DesignDocument{};

    emit transactionStateChanged(false, {});
    emitUndoRedoIfChanged();
}

} // namespace Command
