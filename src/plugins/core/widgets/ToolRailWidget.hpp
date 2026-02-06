#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QHash>
#include <QtCore/QString>

#include "core/api/SidebarToolSpec.hpp"

class QVBoxLayout;
class QToolButton;

namespace Core {

class SidebarModel;

class ToolRailWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit ToolRailWidget(SidebarModel* model,
							SidebarSide side,
							SidebarFamily family,
							QWidget* parent = nullptr);

private slots:
	void rebuild();
	void onToolClicked();

private:
	struct ButtonInfo {
		QString id;
		SidebarRegion region = SidebarRegion::Exclusive;
	};

	QToolButton* makeButton(const QString& id);

	SidebarModel* m_model = nullptr;
	SidebarSide m_side = SidebarSide::Left;
	SidebarFamily m_family = SidebarFamily::Vertical;

	QVBoxLayout* m_top = nullptr;
	QVBoxLayout* m_bottom = nullptr;

	QHash<QToolButton*, ButtonInfo> m_btnInfo;
};

} // namespace Core
