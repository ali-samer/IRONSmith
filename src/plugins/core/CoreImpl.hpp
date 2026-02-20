// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QPointer>

#include "core/ICore.hpp"
#include "core/ui/IUiHost.hpp"

class QMainWindow;
class QWidget;

namespace Core {
namespace Internal { class MainWindow; }
class FrameWidget;
class UiHostImpl;

class CoreImpl final : public ICore
{
	Q_OBJECT

public:
	explicit CoreImpl(QObject* parent = nullptr);
	~CoreImpl() override;

	void setCentralWidget(QWidget* widget) override;
	void open() override;
	IUiHost* uiHost() const;

private:
	void ensureWindowCreated();

private:
	QPointer<Internal::MainWindow> m_mainWindow;
	QPointer<FrameWidget> m_frame;
	QPointer<IUiHost>     m_uiHost;

	bool m_openCalled = false;
};

} // namespace Core
