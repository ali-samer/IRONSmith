// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "codeeditor/api/ICodeEditorService.hpp"

#include <utils/Result.hpp>

#include <QtCore/QHash>
#include <QtCore/QPointer>

namespace Core {
class IUiHost;
}

namespace CodeEditor::Internal {

class CodeEditorServiceImpl final : public CodeEditor::Api::ICodeEditorService {
	Q_OBJECT

public:
	explicit CodeEditorServiceImpl(QObject* parent = nullptr);

	void setUiHost(Core::IUiHost* uiHost);

	Utils::Result openFile(const CodeEditor::Api::CodeEditorOpenRequest &request,
							CodeEditor::Api::CodeEditorSessionHandle &outHandle) override;
	Utils::Result closeFile(const CodeEditor::Api::CodeEditorSessionHandle &handle,
							CodeEditor::Api::CodeEditorCloseReason reason) override;

	CodeEditor::Api::CodeEditorSessionHandle activeFile() const override;
	bool hasOpenFile() const override;

	QWidget* createQuickView(const CodeEditor::Api::CodeEditorQuickViewRequest& request,
							 QWidget* parent = nullptr) override;

	bool supportsLanguage(const QString &languageId) const override;

private:
	struct SessionState final {
		Api::CodeEditorSessionHandle handle;
	};

	static QString normalizePath(const QString& path);
	static QString resolveLanguageId(const QString& hint, const QString& absolutePath);
	static Utils::Result readTextFile(const QString& absolutePath, QString& outText);

	QPointer<Core::IUiHost> m_uiHost;
	QHash<QString, SessionState> m_sessionsById;
	QHash<QString, QString> m_sessionIdByPath;
	CodeEditor::Api::CodeEditorSessionHandle m_active;
};
} // namespace CodeEditor::Internal