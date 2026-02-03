#pragma once

#include "command/CommandPluginGlobal.hpp"
#include "command/DesignCommand.hpp"
#include "command/CommandResult.hpp"

#include <designmodel/DesignDocument.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace Command {

class COMMANDPLUGIN_EXPORT CommandDispatcher final : public QObject {
	Q_OBJECT

public:
	explicit CommandDispatcher(QObject* parent = nullptr);

	const DesignModel::DesignDocument& document() const noexcept { return m_doc; }
	void setDocument(DesignModel::DesignDocument doc);

	bool canUndo() const noexcept { return !m_undo.isEmpty(); }
	bool canRedo() const noexcept { return !m_redo.isEmpty(); }

	CommandResult apply(const DesignCommand& command);

	CommandResult undo();
	CommandResult redo();

	void beginTransaction(QString label = {});
	void commitTransaction();
	void rollbackTransaction();

	bool inTransaction() const noexcept { return m_txDepth > 0; }
	QString currentTransactionLabel() const { return m_txLabel; }

signals:
	void documentChanged(const DesignModel::DesignDocument& doc);
	void commandApplied(const QString& name, const CommandResult& result);
	void undoRedoStateChanged(bool canUndo, bool canRedo);
	void transactionStateChanged(bool inTransaction, const QString& label);

private:
	void emitUndoRedoIfChanged();
	void pushUndoSnapshotIfNeeded();

	DesignModel::DesignDocument m_doc{};

	QVector<DesignModel::DesignDocument> m_undo;
	QVector<DesignModel::DesignDocument> m_redo;

	int m_txDepth{0};
	QString m_txLabel;
	bool m_txTouched{false};
	DesignModel::DesignDocument m_txBase{};
};

} // namespace Command