#pragma once

#include <QtCore/QLineF>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QVector>

namespace Canvas {

struct FabricOverlay
{
    QVector<QPointF> nodes; // world-space node positions
    QVector<QLineF> edges;  // world-space edge segments
};

class FabricRouter final
{
public:
    struct Params
    {
        double obstacleClearance = 2.0;
    };

    static FabricOverlay buildOverlay(const QVector<double>& xs,
                                      const QVector<double>& ys,
                                      const QVector<QRectF>& obstacles,
                                      const Params& params);

	static FabricOverlay buildOverlay(const QVector<double>& xs,
									  const QVector<double>& ys,
									  const QVector<QRectF>& obstacles);

    static QVector<QPointF> route(const QPointF& start,
                                  const QPointF& end,
                                  const QVector<double>& xs,
                                  const QVector<double>& ys,
                                  const QVector<QRectF>& obstacles,
                                  const Params& params);

	static QVector<QPointF> route(const QPointF& start,
								  const QPointF& end,
								  const QVector<double>& xs,
								  const QVector<double>& ys,
								  const QVector<QRectF>& obstacles);
};

} // namespace Canvas
