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

    visualizeGraph(graph, &coloring, "initial.dot", "initial.png", "dot", "png", false);
    return {palette, coloring, usedColors};
}

Node *selectNextNode(const Graph &graph, const ColoringMap &coloring)
{
    Node *bestV = nullptr;
    int bestIncident = 0;
    for (auto *v : graph.getNodes())
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
    return bestV;
}

std::unordered_map<Node *, Color> HillClimbingColoring::run(Graph &graph, int iterations)
{
    auto [palette, coloring, usedColors] = initialState(graph, rng_);
    int conflicts = computeConflicts(graph, coloring);
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    for (int it = 0; it < iterations && conflicts > 0; ++it)
    {
        // pick vertex with a conflict (best: the one with highest incident conflicts)
        Node *bestV = selectNextNode(graph, coloring);
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

            int newConflicts = conflicts - oldInc + newInc;
            if (newConflicts <= conflicts)
            {
                usedColors[oldColor]--;
                coloring[bestV] = palette.getColor(c);
                usedColors[c]++;
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
            break;
        }
    }

    std::cout << "Final conflicts: " << conflicts << "\n";
    return coloring;
}

std::unordered_map<Node *, Color> SimulatedAnnealing::run(Graph &graph, int iterations)
{
    auto [palette, coloring, usedColors] = initialState(graph, rng_);
    int conflicts = computeConflicts(graph, coloring);
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    // sanity: if palette has only one color nothing to do
    if (palette.size() <= 1)
    {
        std::cout << "Palette too small, nothing to do.\n";
        return coloring;
    }

    for (int t = 1; t <= iterations && conflicts > 0; ++t)
    {
        double T = schedule_(t); // keep as double, don't truncate
        if (T <= 1e-12)
        { // stop when temperature effectively zero
            std::cout << "Temperature ~ 0, stopping.\n";
            break;
        }

        // 1) choose a vertex that contributes to conflicts (highest incident conflicts)
        Node *bestV = selectNextNode(graph, coloring);

        if (bestV == nullptr)
        {
            // no conflicting vertex -> solution found
            std::cout << "No conflicting vertices at iteration " << t << ".\n";
            break;
        }

        // 2) pick a different color index for bestV (safe selection)
        int oldColorIdx = coloring[bestV].index;
        int selectedColorIdx = oldColorIdx;
        // pick a different color index uniformly
        std::uniform_int_distribution<int> dist(0, static_cast<int>(palette.size() - 2));
        {
            int r = dist(rng_);
            if (r >= oldColorIdx)
                ++r; // shift values >= oldColorIdx to skip old color
            selectedColorIdx = r;
        }

        // 3) evaluate local delta (incident conflicts on bestV)
        int oldInc = countNodeConflicts(bestV, coloring); // uses oldColorIdx
        Color savedOldColor = coloring[bestV];            // save object to revert if needed

        // temporarily apply candidate color
        coloring[bestV] = palette.getColor(selectedColorIdx);
        int newInc = countNodeConflicts(bestV, coloring);

        int dE = newInc - oldInc; // positive -> worse (more conflicts)

        bool accept = false;
        if (dE <= 0)
        {
            // improvement (or equal) -> accept
            accept = true;
        }
        else
        {
            // worse -> accept with probability exp(-dE / T)
            double acceptanceProb = std::exp(-static_cast<double>(dE) / T);
            std::uniform_real_distribution<double> probDist(0.0, 1.0);
            accept = (probDist(rng_) < acceptanceProb);
        }

        if (accept)
        {
            // commit: update global conflict count using local delta
            conflicts = conflicts - oldInc + newInc;
        }
        else
        {
            // revert coloring
            coloring[bestV] = savedOldColor;
            // conflicts unchanged
        }

        // optional debug: every N iterations recompute full conflicts to ensure consistency
        if ((t & 127) == 0)
        {
            int check = computeConflicts(graph, coloring);
            if (check != conflicts)
            {
                std::cerr << "WARNING: conflict counter mismatch: cached=" << conflicts
                          << " recomputed=" << check << " at t=" << t << "\n";
                conflicts = check; // resync to safe state
            }
        }
    }

    std::cout << "Final conflicts: " << conflicts << "\n";
    return coloring;
}

double SimulatedAnnealing::schedule_(int t)
{
    return 100.0 * std::pow(0.95, t);
}
