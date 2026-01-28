#pragma once

#include <QtCore/QString>

namespace Core {

// Where the tool lives.
enum class SidebarSide { Left, Right };

// Which “dock family” this belongs to.
enum class SidebarFamily { Vertical, Horizontal };

// How panels behave when the tool is activated.
enum class SidebarRegion { Exclusive, Additive };

// Where the tool button appears within its family rail.
enum class SidebarRail { Top, Bottom };

struct SidebarToolSpec
{
	// Required, stable id. Example: "IRONSmith.group.member"
	QString id;

	// Caption shown under/next-to the icon (depending on styling).
	QString title;

	// Resource path: ":/ui/icons/...."
	// Keep it as a string in the API so plugins don’t depend on rendering details.
	// TODO: Refactor type to use QIcon instead
	// TODO: Attempt to make styling belong to src/app/
	QString iconResource;

	// Placement
	SidebarSide side = SidebarSide::Left;
	SidebarFamily family = SidebarFamily::Vertical;
	SidebarRegion region = SidebarRegion::Exclusive;
	SidebarRail rail = SidebarRail::Top;

	// Ordering within its rail. Default 0.
	int order = 0;

	// Optional field: tooltip (empty means “use title”).
	QString toolTip;
};

} // namespace Core