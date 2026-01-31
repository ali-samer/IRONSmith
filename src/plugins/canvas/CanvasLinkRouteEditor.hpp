#pragma once

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QVector>

namespace Canvas {

class LinkRouteEditor final
{
public:
    struct Result
    {
        QVector<QPointF> worldPoints;
        bool ok = false;

        int runStartPointIndex = -1; // inclusive
        int runEndPointIndex = -1;   // inclusive
        bool horizontalRun = false;
        double snappedCoord = 0.0;   // snapped y (horizontal) or x (vertical)
    };

    static Result shiftSegmentToNearestLane(const QVector<QPointF>& worldPolyline,
                                           int segIndex,
                                           const QPointF& mouseWorld,
                                           const QVector<double>& xs,
                                           const QVector<double>& ys,
                                           const QVector<QRectF>& obstacles,
                                           double clearance);
};

} // namespace Canvas
