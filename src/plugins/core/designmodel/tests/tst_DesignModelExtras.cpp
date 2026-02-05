#include <gtest/gtest.h>

#include "designmodel/DesignDocument.hpp"
#include "designmodel/DesignExtras.hpp"

using namespace DesignModel;

TEST(Extras, BuilderStoresAndLooksUp) {
	DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
	DesignDocument::Builder b(DesignSchemaVersion::current(), md);

	const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(0, 0)), "A");
	const PortId out = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
	const PortId in  = b.createPort(a, PortDirection::Input,  PortType(PortTypeKind::Stream), "in");
	const LinkId l = b.createLink(out, in, "loop");

	const NetId n = b.createNet("net0", QVector<LinkId>{l});
	const AnnotationId ann = b.createAnnotation(AnnotationKind::Note, "hello",
												QVector<BlockId>{a}, {}, QVector<LinkId>{l}, {}, "debug");
	const RouteId r = b.createRoute(l, QVector<TileCoord>{TileCoord(0,0), TileCoord(0,1)});

	const DesignDocument doc = b.freeze();
	ASSERT_TRUE(doc.isValid());

	ASSERT_NE(doc.tryNet(n), nullptr);
	ASSERT_NE(doc.tryAnnotation(ann), nullptr);
	ASSERT_NE(doc.tryRoute(r), nullptr);

	EXPECT_EQ(doc.netIds().size(), 1);
	EXPECT_EQ(doc.annotationIds().size(), 1);
	EXPECT_EQ(doc.routeIds().size(), 1);

	EXPECT_EQ(doc.tryNet(n)->links().size(), 1);
	EXPECT_EQ(doc.tryAnnotation(ann)->text(), "hello");
	EXPECT_EQ(doc.tryRoute(r)->path().size(), 2);
}