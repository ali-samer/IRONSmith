#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtCore/QRect>
#include <vector>

namespace Canvas {

template <typename Tag>
class StrongId final
{
public:
	using value_type = quint64;

	constexpr StrongId() = default;
	explicit constexpr StrongId(value_type v) : m_value(v) {}

	constexpr value_type value() const { return m_value; }
	constexpr bool isValid() const { return m_value != 0; }

	explicit operator bool() const { return isValid(); }

	friend constexpr bool operator==(StrongId a, StrongId b) { return a.m_value == b.m_value; }
	friend constexpr bool operator!=(StrongId a, StrongId b) { return a.m_value != b.m_value; }

private:
	value_type m_value = 0;
};

struct BlockIdTag {};
struct PortIdTag {};
struct LinkIdTag {};
struct ObjectIdTag {};

using BlockId = StrongId<BlockIdTag>;
using PortId  = StrongId<PortIdTag>;
using LinkId  = StrongId<LinkIdTag>;
using ObjectId = StrongId<ObjectIdTag>;

struct CANVAS_EXPORT GridCoord final {
	int x = 0;
	int y = 0;
};

struct CANVAS_EXPORT GridSize final {
	int w = 1;
	int h = 1;
};

struct CANVAS_EXPORT FabricCoord final {
	int x = 0;
	int y = 0;
};

// struct Port final {
// 	PortId   id{};
// 	BlockId  owner{};
// 	PortCap  cap = PortCap::InOut;
// 	PortSide side = PortSide::East;
// 	uint16_t slot = 0;
// 	QString  tag;
// };
//
// struct Block final {
// 	BlockId   id{};
// 	QString   typeTag;
// 	QString   label;
// 	GridCoord anchor{};
// 	GridSize  footprint{1,1};
// 	std::vector<PortId> ports;
// };
//
// struct Link final {
// 	LinkId   id{};
// 	PortId   src{};
// 	PortId   dst{};
// 	std::vector<FabricCoord> steps;
// 	QString  tag;
// };
//

#define DEFINE_QHASH_OVERLOAD(Id) \
inline uint qHash(Id id, uint seed = 0) noexcept {  \
	return ::qHash(id.value(), seed);				\
}

DEFINE_QHASH_OVERLOAD(Canvas::BlockId)
DEFINE_QHASH_OVERLOAD(Canvas::PortId)
DEFINE_QHASH_OVERLOAD(Canvas::LinkId)
DEFINE_QHASH_OVERLOAD(Canvas::ObjectId)

#undef DEFINE_QHASH_OVERLOAD

} // namespace Canvas
