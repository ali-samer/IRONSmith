#pragma once

#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointF>

#include <array>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Canvas::Internal {

struct CoordBounds final {
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
};

class WireRouter final
{
public:
    explicit WireRouter(const CanvasRenderContext& ctx);

    std::vector<FabricCoord> routeCoords(const FabricCoord& start, const FabricCoord& goal) const;
    std::vector<FabricCoord> routeCoordsViaWaypoints(const std::vector<FabricCoord>& waypoints) const;

    std::vector<QPointF> routeFabricPath(const QPointF& aFabric, const QPointF& bFabric) const;
    std::vector<QPointF> routeViaWaypoints(const std::vector<FabricCoord>& waypoints,
                                           const QPointF& aFabric,
                                           const QPointF& bFabric) const;

private:
    static constexpr int kTurnPenalty = 3;
    static constexpr int kDirNone = -1;

    struct Node final {
        int x = 0;
        int y = 0;
        int dir = kDirNone;
        int g = 0;
        int f = 0;
    };

    struct StateKey final {
        int x = 0;
        int y = 0;
        int dir = kDirNone;
    };

    struct NodeCompare final {
        bool operator()(const Node& a, const Node& b) const
        {
            if (a.f != b.f)
                return a.f > b.f;
            if (a.g != b.g)
                return a.g > b.g;
            if (a.dir != b.dir)
                return a.dir > b.dir;
            if (a.y != b.y)
                return a.y > b.y;
            return a.x > b.x;
        }
    };

    struct StateKeyHash final {
        size_t operator()(const StateKey& key) const noexcept
        {
            size_t h1 = std::hash<int>{}(key.x);
            size_t h2 = std::hash<int>{}(key.y);
            size_t h3 = std::hash<int>{}(key.dir);
            h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            return h1;
        }
    };

    struct StateKeyEq final {
        bool operator()(const StateKey& a, const StateKey& b) const noexcept
        {
            return a.x == b.x && a.y == b.y && a.dir == b.dir;
        }
    };

    using OpenSet = std::priority_queue<Node, std::vector<Node>, NodeCompare>;
    using ScoreMap = std::unordered_map<StateKey, int, StateKeyHash, StateKeyEq>;
    using CameFromMap = std::unordered_map<StateKey, StateKey, StateKeyHash, StateKeyEq>;

    struct SearchState final {
        CoordBounds bounds{};
        StateKey startKey{};
        FabricCoord goal{};
        int visited = 0;
    };

    std::vector<FabricCoord> routeSegment(const FabricCoord& start, const FabricCoord& goal) const;
    std::vector<FabricCoord> aStarPath(const FabricCoord& start, const FabricCoord& goal) const;
    std::vector<FabricCoord> trySimplePath(const FabricCoord& start, const FabricCoord& goal) const;

    double effectiveStep() const;
    SearchState initSearch(const FabricCoord& start, const FabricCoord& goal, double step) const;

    bool isStaleNode(const ScoreMap& gScore, const Node& cur, const StateKey& curKey) const;
    void expandNode(SearchState& state, OpenSet& open,
                    ScoreMap& gScore, CameFromMap& cameFrom,
                    const Node& cur, const StateKey& curKey) const;
    void tryEnqueueNeighbor(SearchState& state, OpenSet& open,
                            ScoreMap& gScore, CameFromMap& cameFrom,
                            const Node& cur, const StateKey& curKey, int dir) const;

    bool isBlocked(const FabricCoord& coord, const FabricCoord& goal) const;
    std::vector<FabricCoord> rebuildPath(const CameFromMap& cameFrom, const StateKey& startKey,
                                         StateKey goalKey) const;

    static std::vector<FabricCoord> directManhattanPath(const FabricCoord& start,
                                                        const FabricCoord& goal);
    static std::vector<FabricCoord> concatSegments(std::vector<FabricCoord> a,
                                                   std::vector<FabricCoord> b);
    std::vector<FabricCoord> smoothPath(const std::vector<FabricCoord>& path) const;

    static bool isAxisAligned(const FabricCoord& a, const FabricCoord& b);
    bool isSegmentClear(const FabricCoord& start, const FabricCoord& end, bool allowEndBlocked) const;

    int stepCost(int prevDir, int nextDir) const;
    std::array<int, 4> orderedDirs(int currentDir) const;
    QPoint dirDelta(int dir) const;

    const CanvasRenderContext& m_ctx;
};

std::vector<QPointF> simplifyCoordsToScene(const std::vector<FabricCoord>& coords,
                                           double step,
                                           const QPointF& aFabric,
                                           const QPointF& bFabric);

} // namespace Canvas::Internal
