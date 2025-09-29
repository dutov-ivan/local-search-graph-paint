// Algorithm implementations moved from header to avoid multiple definition issues
#include "algorithms.h"
#include "visualization.h"

#include <vector>
#include <random>
#include <climits>

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

void greedyRemoveConflicts(Graph &graph, ColoringMap &coloring, ColorPalette &palette)
{
    for (auto *node : graph.getNodes())
    {
        int conflicts = countNodeConflicts(node, coloring);
        if (conflicts > 0)
        {
            for (int colorIdx = 0; colorIdx < palette.size(); ++colorIdx)
            {
                if (colorIdx == coloring[node].index)
                    continue; // skip current color

                // Temporarily assign new color and count conflicts
                coloring[node] = palette.getColor(colorIdx);
                int newConflicts = countNodeConflicts(node, coloring);

                if (conflicts == 0)
                {
                    break;
                }
            }
        }
    }
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

std::tuple<ColorPalette, ColoringMap, std::unordered_map<int, int>> initialState(Graph &graph, std::mt19937 &rng_)
{
    const auto &nodes = graph.getNodes();

    // compute maxDegree once
    int maxDegree = 0;
    for (auto *n : nodes)
        maxDegree = std::max<int>(maxDegree, n->getNeighbors().size());

    ColorPalette palette(maxDegree + 1); // enough colors for any node's incident edges

    ColoringMap coloring;
    std::unordered_map<int, int> usedColors;
    std::uniform_int_distribution<int> colorDist(0, static_cast<int>(palette.size()) - 1);
    for (auto *node : nodes)
    {
        coloring[node] = palette.getColor(colorDist(rng_));
        usedColors[coloring[node].index]++;
    }

    if (nodes.size() <= 2000)
        visualizeGraph(graph, &coloring, "initial.dot", "initial.png", "dot", "png", false);
    return {palette, coloring, usedColors};
}

Node *selectNextNode(const Graph &graph, const ColoringMap &coloring, const std::unordered_map<int, int> &usedColors)
{
    Node *bestV = nullptr;
    int bestIncident = -1;
    int bestColorUse = INT_MAX; // we prefer the least-used color
    for (auto *v : graph.getNodes())
    {
        auto itv = coloring.find(v);
        if (itv == coloring.end())
            continue;
        int vcol = itv->second.index;

        // how many neighbors share v's color
        int incident = 0;
        for (auto *nbr : v->getNeighbors())
        {
            auto itn = coloring.find(nbr);
            if (itn != coloring.end() && itn->second.index == vcol)
                ++incident;
        }

        // skip vertices that are currently conflict-free
        if (incident == 0)
            continue;

        int colorUse = 0;
        if (auto itUse = usedColors.find(vcol); itUse != usedColors.end())
            colorUse = itUse->second;

        // choose the vertex whose conflicts are the most; tie-breaker: least used color
        if (incident > bestIncident || (incident == bestIncident && colorUse < bestColorUse))
        {
            bestColorUse = colorUse;
            bestIncident = incident;
            bestV = v;
        }
    }
    return bestV;
}

int computeH(int conflicts, int usesOfColor)
{
    return conflicts * 1000 + usesOfColor;
}

std::unordered_map<Node *, Color> HillClimbingColoring::run(Graph &graph, int iterations)
{
    auto [palette, coloring, usedColors] = initialState(graph, rng_);
    int conflicts = computeConflicts(graph, coloring);
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    for (int it = 0; it < iterations && conflicts > 0; ++it)
    {
        Node *bestV = selectNextNode(graph, coloring, usedColors);
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

        int oldH = computeH(conflicts, usedColors[oldColor]);
        int bestH = oldH;
        int bestConflicts = conflicts;
        int bestColor = oldColor;

        bool improved = false;
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

            int newConflicts = conflicts - oldInc + newInc;
            int newH = computeH(newConflicts, usedColors[c]);

            if (newH < bestH)
            {
                bestH = newH;
                bestConflicts = newConflicts;
                bestColor = c;
                improved = true;
            }
        }

        if (improved)
        {
            conflicts = bestConflicts;

            usedColors[oldColor]--;
            coloring[bestV] = palette.getColor(bestColor);
            usedColors[bestColor]++;
        }
        else
        {
            break;
        }
    }

    std::cout << "Final conflicts: " << conflicts << "\n";

    if (conflicts > 0)
    {
        std::cout << "Performing greedy conflict removal...\n";
        greedyRemoveConflicts(graph, coloring, palette);
    }

    return coloring;
}

std::unordered_map<Node *, Color> SimulatedAnnealing::run(Graph &graph, int iterations)
{
    auto [palette, coloring, usedColors] = initialState(graph, rng_);
    int conflicts = computeConflicts(graph, coloring);
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    for (int t = 1; t <= iterations && conflicts > 0; ++t)
    {
        double T = schedule_(t); // keep as double, don't truncate
        if (T <= 1e-12)
        { // stop when temperature effectively zero
            std::cout << "Temperature ~ 0, stopping.\n";
            break;
        }

        // 1) choose a vertex that contributes to conflicts (highest incident conflicts)
        Node *bestV = selectNextNode(graph, coloring, usedColors);

        if (bestV == nullptr)
        {
            // no conflicting vertex -> solution found
            std::cout << "No conflicting vertices at iteration " << t << ".\n";
            break;
        }

        int oldColorIdx = coloring[bestV].index;
        int selectedColorIdx = oldColorIdx;
        std::uniform_int_distribution<int> dist(0, static_cast<int>(palette.size() - 2));
        {
            int r = dist(rng_);
            if (r >= oldColorIdx)
                ++r;
            selectedColorIdx = r;
        }

        int oldInc = countNodeConflicts(bestV, coloring);
        Color savedOldColor = coloring[bestV];

        coloring[bestV] = palette.getColor(selectedColorIdx);
        int newInc = countNodeConflicts(bestV, coloring);

        int newConflicts = conflicts - oldInc + newInc;
        int oldH = computeH(conflicts, usedColors[oldColorIdx]);
        int newH = computeH(newConflicts, usedColors[selectedColorIdx]);
        int dE = newH - oldH;

        bool accept = false;
        if (dE <= 0)
        {
            accept = true;
        }
        else
        {
            double acceptanceProb = std::exp(-static_cast<double>(dE) / T);
            std::uniform_real_distribution<double> probDist(0.0, 1.0);
            accept = (probDist(rng_) < acceptanceProb);
        }

        if (accept)
        {
            conflicts = conflicts - oldInc + newInc;
            usedColors[oldColorIdx]--;
            usedColors[selectedColorIdx]++;
        }
        else
        {
            coloring[bestV] = savedOldColor;
        }
    }

    std::cout << "Final conflicts: " << conflicts << "\n";

    if (conflicts > 0)
    {
        std::cout << "Performing greedy conflict removal...\n";
        greedyRemoveConflicts(graph, coloring, palette);
    }

    return coloring;
}

double SimulatedAnnealing::schedule_(int t)
{
    return 100.0 * std::pow(0.95, t);
}
