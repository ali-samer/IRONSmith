// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "codeeditor/internal/CodeEditorServiceImpl.hpp"

#include "codeeditor/CodeEditorGlobal.hpp"
#include "codeeditor/Constants.hpp"

#include <utils/PathUtils.hpp>
#include <utils/Comparisons.hpp>
#include <core/ui/IUiHost.hpp>

#include <QFontDatabase>
#include <QUuid>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QPlainTextEdit>

namespace CodeEditor::Internal {

CodeEditorServiceImpl::CodeEditorServiceImpl(QObject* parent)
	: CodeEditor::Api::ICodeEditorService(parent) {}

void CodeEditorServiceImpl::setUiHost(Core::IUiHost* uiHost)
{
	if (!uiHost) {
		qCWarning(ceditorlog) << "CodeEditorServiceImpl::setUiHost: uiHost is null";
		return;
	}

	m_uiHost = uiHost;
}

Utils::Result CodeEditorServiceImpl::openFile(const CodeEditor::Api::CodeEditorOpenRequest &request,
												CodeEditor::Api::CodeEditorSessionHandle &outHandle)
{
	outHandle = {};

	const QString absPath = Utils::PathUtils::normalizePath(request.filePath);
	if (absPath.isEmpty())
		return Utils::Result::failure(QStringLiteral("CodeEditor: file path empty."));

	const QFileInfo info{absPath};
	if (!info.exists() || !info.isFile())
		return Utils::Result::failure(QStringLiteral("CodeEditor: file does not exist: %1.").arg(absPath));

	const QString existingId = m_sessionIdByPath.value(absPath);
	if (!existingId.isEmpty()) {
		const auto it = m_sessionsById.constFind(existingId);
		if (it != m_sessionsById.constEnd()) {
			outHandle = it.value().handle;
			if (request.activate && (!m_active.isValid() || m_active.id != outHandle.id)) {
				m_active = outHandle;
				emit activeFileChanged(m_active);
			}
			return Utils::Result::success();
		}
	}

	SessionState state;
	state.handle.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	state.handle.filePath = absPath;
	state.handle.languageId = resolveLanguageId(request.languageHint, absPath);
	state.handle.readOnly = request.readOnly;

	m_sessionIdByPath.insert(state.handle.filePath, state.handle.id);
	m_sessionsById.insert(state.handle.id, state);

	outHandle = state.handle;
	emit fileOpened(outHandle);

	if (request.activate) {
		m_active = outHandle;
		emit activeFileChanged(m_active);
	}

	qCInfo(ceditorlog) << "CodeEditor: opened file session: " << outHandle.filePath << outHandle.languageId;
	return Utils::Result::success();
}

Utils::Result CodeEditorServiceImpl::closeFile(const CodeEditor::Api::CodeEditorSessionHandle &handle,
	CodeEditor::Api::CodeEditorCloseReason reason)
{
	if (!handle.isValid())
		return Utils::Result::failure(QStringLiteral("CodeEditor: invalid session handle."));

	const auto it = m_sessionsById.find(handle.id);
	if (it == m_sessionsById.end())
		return Utils::Result::failure(QStringLiteral("CodeEditor: session is not active."));

	const CodeEditor::Api::CodeEditorSessionHandle closed = it.value().handle;
	m_sessionIdByPath.remove(closed.filePath);
	m_sessionsById.erase(it);

	emit fileClosed(closed, reason);

	if (m_active.isValid() && m_active.id == closed.id) {
		m_active = {};
		if (!m_sessionsById.isEmpty())
			m_active = m_sessionsById.cbegin().value().handle;
		emit activeFileChanged(m_active);
	}

	return Utils::Result::success();
}

CodeEditor::Api::CodeEditorSessionHandle CodeEditorServiceImpl::activeFile() const
{
	return m_active;
}

bool CodeEditorServiceImpl::hasOpenFile() const
{
	return m_active.isValid();
}

QWidget * CodeEditorServiceImpl::createQuickView(const CodeEditor::Api::CodeEditorQuickViewRequest &request,
	QWidget *parent)
{
	auto* view = new QPlainTextEdit(parent);
	view->setObjectName(QStringLiteral("CodeEditorQuickView"));
	view->setReadOnly(true);
	view->setUndoRedoEnabled(false);
	view->setLineWrapMode(QPlainTextEdit::NoWrap);
	view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	const QString absolutePath = Utils::PathUtils::normalizePath(request.filePath);
	if (absolutePath.isEmpty()) {
		view->setPlainText(QStringLiteral("CodeEditor quick view: empty path."));
		return view;
	}

	QString text;
	const Utils::Result result = readTextFile(absolutePath, text);
	if (!result) {
		view->setPlainText(QStringLiteral("Unable to open '%1'.\n\n%2")
							   .arg(absolutePath, result.errors.join(QStringLiteral("\n"))));
		return view;
	}

	view->setPlainText(text);
	return view;
}

bool CodeEditorServiceImpl::supportsLanguage(const QString &languageId) const {
	const QString id = languageId.trimmed().toLower();
	if (id.isEmpty())
		return false;

	return Constants::kSupportedLanguages.contains(id);
}

QString CodeEditorServiceImpl::resolveLanguageId(const QString &hint, const QString &absolutePath) {
	const QString cleanedHint = hint.trimmed().toLower();
	if (!cleanedHint.isEmpty())
		return cleanedHint;

	const QString suffix = Utils::PathUtils::extension(cleanedHint);
	if (Utils::isOneOf(suffix, "c", "h"))
		return Constants::kLanguageIdC;

	if (Utils::isOneOf(suffix, "cc", "cpp", "cxx", "hpp", "hh", "hxx"))
		return Constants::kLanguageIdCpp;

	if (suffix == "py")
		return Constants::kLanguageIdPython;

	if (suffix == "json")
		return Constants::kLanguageIdJson;

	if (suffix == "xml")
		return Constants::kLanguageIdXml;

	return Constants::kLanguageIdText;
}

Utils::Result CodeEditorServiceImpl::readTextFile(const QString &absolutePath, QString &outText) {
	outText.clear();

	QFile file(absolutePath);
	if (!file.open(QIODevice::ReadOnly))
		return Utils::Result::failure(QStringLiteral("Failed to open file: %1").arg(absolutePath));

	const QByteArray bytes = file.read(Constants::kQuickViewMaxBytes + 1);
	if (bytes.size() > Constants::kQuickViewMaxBytes) {
		return Utils::Result::failure(
			QStringLiteral("Quick view limit exceeded (%1 bytes).").arg(Constants::kQuickViewMaxBytes));
	}

	outText = QString::fromUtf8(bytes);
	return Utils::Result::success();
}
} // namespace CodeEditor::Internal