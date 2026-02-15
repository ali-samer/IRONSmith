// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtWidgets/QApplication>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>

namespace Ui {

struct UiStyle final
{
	static constexpr int PadS = 6;
	static constexpr int PadM = 10;

	static constexpr int MenuBarHeightPx = 32;
	static constexpr int MenuBarHMargin = 8;
	static constexpr int MenuBarButtonSpacing = 2;

	static constexpr int RibbonHostHeightPx = 100;
	static constexpr int RibbonIconLargePx = 64;
	static constexpr int RibbonIconMediumPx = 48;
	static constexpr int RibbonIconSmallPx = 32;
	static constexpr int RibbonIconDefaultPx = RibbonIconMediumPx;
	static constexpr int RibbonPageHPaddingPx         = 8;
	static constexpr int RibbonPageVPaddingPx         = 4;

	static constexpr int RibbonGroupContentHPaddingPx = 6;
	static constexpr int RibbonGroupContentVPaddingPx = 4;
	static constexpr int TopBarHeight = 28;
	static constexpr int BottomBarHeight = 28;

	static constexpr int SidebarWidth = 56;
	static constexpr int SidebarMinWidth = 48;

	static QString loadStylesheet()
	{
		const QByteArray overridePath = qgetenv("IRONSMITH_QSS");
		if (!overridePath.isEmpty()) {
			QFile f(QString::fromUtf8(overridePath));
			if (f.open(QIODevice::ReadOnly | QIODevice::Text))
				return QString::fromUtf8(f.readAll());
			qWarning() << "IRONSmith: failed to open IRONSMITH_QSS:" << overridePath;
		}

		QFile f(":/ui/Default.qss");
		if (f.open(QIODevice::ReadOnly | QIODevice::Text))
			return QString::fromUtf8(f.readAll());

		qWarning() << "IRONSmith: failed to open resource stylesheet :/ui/Default.qss";
		return {};
	}

	static void applyAppStyle(QApplication& app)
	{
		static QString s_fontFamily;
		if (s_fontFamily.isEmpty()) {
			const QStringList fontFiles = {
				":/ui/fonts/dejavu-sans-mono/ttf/DejaVuSansMono.ttf",
				":/ui/fonts/dejavu-sans-mono/ttf/DejaVuSansMono-Bold.ttf",
				":/ui/fonts/dejavu-sans-mono/ttf/DejaVuSansMono-Oblique.ttf",
				":/ui/fonts/dejavu-sans-mono/ttf/DejaVuSansMono-BoldOblique.ttf"
			};

			int regularId = -1;
			for (const auto& file : fontFiles) {
				const int id = QFontDatabase::addApplicationFont(file);
				if (regularId < 0 && file.contains("DejaVuSansMono.ttf"))
					regularId = id;
			}

			if (regularId >= 0) {
				const QStringList families = QFontDatabase::applicationFontFamilies(regularId);
				if (!families.isEmpty())
					s_fontFamily = families.front();
			}

			if (s_fontFamily.isEmpty())
				qWarning() << "IRONSmith: failed to load application font.";
		}

		if (!s_fontFamily.isEmpty()) {
			QFont f = app.font();
			if (f.family() != s_fontFamily) {
				f.setFamily(s_fontFamily);
				app.setFont(f);
			}
		}

		static QString s_lastApplied;
		const QString qss = loadStylesheet();
		if (qss.isEmpty())
			return;

		if (qss == s_lastApplied)
			return;

		s_lastApplied = qss;
		qApp->setStyleSheet(qss);
	}
};

} // namespace Ui
