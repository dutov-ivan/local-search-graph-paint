// Algorithm implementations moved from header to avoid multiple definition issues
#include "algorithms.h"
#include "visualization.h"

#include <vector>
#include <random>

// Deterministic extra color generator (in case preset palette is insufficient)
static Color generateExtraColor(int order)
{
    auto channel = [order](int multiplier, int offset)
    {
        int value = (multiplier * order + offset) % 256;
        if (value < 0)
            value += 256;
        return value;
    };

    return {order,
            channel(97, 53),
            channel(193, 101),
            channel(151, 197)};
}

// Compute how many distinct neighbor colors a node sees
int computeNodeSaturation(Node *node, const ColoringMap &coloring)
{
    std::unordered_set<int> neighborColors;
    for (auto &neighbor : node->getNeighbors())
    {
        auto it = coloring.find(neighbor);
        if (it != coloring.end())
        {
            neighborColors.insert(it->second.index);
        }
    }
    return static_cast<int>(neighborColors.size());
}

std::unordered_map<Node *, int> computeSaturation(Graph &graph, const ColoringMap &coloring)
{
    std::unordered_map<Node *, int> saturationMap;
    for (auto &node : graph.getNodes())
    {
        saturationMap[node] = computeNodeSaturation(node, coloring);
    }
    return saturationMap;
}

static int countNodeConflicts(Node *node, const ColoringMap &coloring)
{
    int conflictCount = 0;
    auto itNode = coloring.find(node);
    if (itNode == coloring.end())
    {
        throw std::runtime_error("Node not found in coloring map");
    }
    int nodeColor = itNode->second.index;
    for (auto *neighbor : node->getNeighbors())
    {
        auto itNeighbor = coloring.find(neighbor);
        if (itNeighbor == coloring.end())
            continue;
        if (nodeColor == itNeighbor->second.index)
            ++conflictCount;
    }
    return conflictCount;
}

// Evaluate number of conflicting edges (endpoints share the same color)
static int computeConflicts(const Graph &graph, const ColoringMap &coloring)
{
    long long conflictCount = 0;
    for (auto *node : graph.getNodes())
    {
        conflictCount += countNodeConflicts(node, coloring);
    }
    return static_cast<int>(conflictCount / 2);
}

std::unordered_map<Node *, Color> HillClimbingColoring::run(Graph &graph, int iterations)
{
    const auto &nodes = graph.getNodes();
    if (nodes.empty())
        return {};

    // compute maxDegree once
    int maxDegree = 0;
    for (auto *n : nodes)
        maxDegree = std::max<int>(maxDegree, n->getNeighbors().size());

    ColorPalette palette(maxDegree + 1); // enough colors for any node's incident edges

    ColoringMap coloring;
    std::uniform_int_distribution<int> colorDist(0, static_cast<int>(palette.size()) - 1);
    for (auto *node : nodes)
        coloring[node] = palette.getColor(colorDist(rng_));

    visualizeGraph(graph, &coloring, "initial.dot", "initial.png", "dot", "png", false);

    int conflicts = computeConflicts(graph, coloring);
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    for (int it = 0; it < iterations && conflicts > 0; ++it)
    {
        // pick vertex with a conflict (best: the one with highest incident conflicts)
        Node *bestV = nullptr;
        int bestIncident = 0;
        for (auto *v : nodes)
        {
            int incident = 0;
            auto itv = coloring.find(v);
            if (itv == coloring.end())
                continue; // or treat as 0
            int vcol = itv->second.index;
            for (auto *nbr : v->getNeighbors())
            {
                auto itn = coloring.find(nbr);
                if (itn != coloring.end() && itn->second.index == vcol)
                    ++incident;
            }
            if (incident > bestIncident)
            {
                bestIncident = incident;
                bestV = v;
            }
        }

        if (!bestV)
            break; // no conflicting vertex -> done

        int oldInc = 0;
        int oldColor = coloring[bestV].index;
        for (auto *nbr : bestV->getNeighbors())
        {
            auto itn = coloring.find(nbr);
            if (itn != coloring.end() && itn->second.index == oldColor)
                ++oldInc;
        }

        bool improved = false;
        // try all colors in palette (or shuffle a subset for randomness)
        for (int c = 0; c < (int)palette.size(); ++c)
        {
            if (c == oldColor)
                continue;

            int newInc = 0;
            for (auto *nbr : bestV->getNeighbors())
            {
                auto itn = coloring.find(nbr);
                if (itn != coloring.end() && itn->second.index == c)
                    ++newInc;
            }

            int newConflicts = conflicts - oldInc + newInc; // local delta on edge count
            if (newConflicts <= conflicts)
            {
                coloring[bestV] = palette.getColor(c);
                conflicts = newConflicts;
                improved = true;
                if (conflicts == 0)
                    break;
                // recompute oldInc for further candidate colors
                oldInc = newInc;
                oldColor = c;
            }
        }

        if (!improved)
        {
            // optional: try random restart or pick different vertex; for now stop
            break;
        }
    }

    std::cout << "Final conflicts: " << conflicts << "\n";
    return coloring;
}
