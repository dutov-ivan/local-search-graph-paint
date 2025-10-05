// Algorithm implementations moved from header to avoid multiple definition issues
#include "algorithms.h"
#include <vector>
#include <random>
#include <climits>
#include <memory>
#include <algorithm>

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

static int countNodeConflicts(const std::shared_ptr<GraphNode> &node, const ColoringMap &coloring)
{
    int conflictCount = 0;
    auto itNode = coloring.find(node);
    if (itNode == coloring.end())
    {
        throw std::runtime_error("Node not found in coloring map");
    }
    int nodeColor = itNode->second.index;
    for (const auto &neighbor : node->getNeighbors())
    {
        auto itNeighbor = coloring.find(neighbor);
        if (itNeighbor == coloring.end())
            continue;
        if (nodeColor == itNeighbor->second.index)
            ++conflictCount;
    }
    return conflictCount;
}

void greedyRemoveConflicts(StateNode &state)
{
    for (const auto &node : state.graph->getNodes())
    {
        int conflicts = countNodeConflicts(node, state.coloring);
        if (conflicts > 0)
        {
            for (int colorIdx = 0; colorIdx < state.palette.size(); ++colorIdx)
            {
                if (colorIdx == state.coloring[node].index)
                    continue; // skip current color

                // Temporarily assign new color and count conflicts
                state.coloring[node] = state.palette.getColor(colorIdx);
                int newConflicts = countNodeConflicts(node, state.coloring);

                if (newConflicts == 0)
                {
                    break;
                }
            }
        }
    }
}

// Evaluate number of conflicting edges (endpoints share the same color)
int computeConflicts(const Graph &graph, const ColoringMap &coloring)
{
    long long conflictCount = 0;
    for (const auto &node : graph.getNodes())
    {
        conflictCount += countNodeConflicts(node, coloring);
    }
    return static_cast<int>(conflictCount / 2);
}

std::shared_ptr<GraphNode> selectNextNode(const StateNode &state)
{
    std::shared_ptr<GraphNode> bestV = nullptr;
    int bestIncident = -1;
    int bestColorUse = INT_MAX; // we prefer the least-used color
    for (const auto &v : state.graph->getNodes())
    {
        auto itv = state.coloring.find(v);
        if (itv == state.coloring.end())
            continue;
        int vcol = itv->second.index;

        // how many neighbors share v's color
        int incident = 0;
        for (const auto &nbr : v->getNeighbors())
        {
            auto itn = state.coloring.find(nbr);
            if (itn != state.coloring.end() && itn->second.index == vcol)
                ++incident;
        }

        // skip vertices that are currently conflict-free
        if (incident == 0)
            continue;

        int colorUse = 0;
        if (auto itUse = state.usedColors.find(vcol); itUse != state.usedColors.end())
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

ColoringMap HillClimbingColoring::run(std::unique_ptr<StateNode> initialState, int iterations)
{
    auto current = std::move(*initialState); // Copy to stack by dereferencing
    int conflicts = current.conflicts;
    std::cout << "Initial conflicts: " << conflicts << std::endl;

    for (int it = 0; it < iterations; ++it)
    {
        auto bestV = selectNextNode(current);
        if (!bestV)
            break; // no conflicting vertex -> done

        int oldColor = current.coloring[bestV].index;
        int oldInc = countNodeConflicts(bestV, current.coloring);

        int oldH = current.computeH();
        int bestH = oldH;
        int bestConflicts = conflicts;
        int bestColor = oldColor;

        bool improved = false;
        for (int c = 0; c < (int)current.palette.size(); ++c)
        {
            if (c == oldColor)
                continue;

            current.coloring[bestV] = current.palette.getColor(c);
            int newInc = countNodeConflicts(bestV, current.coloring);
            int newConflicts = conflicts - oldInc + newInc;
            current.conflicts = newConflicts;

            int newH = current.computeH();

            // Revert changes
            current.coloring[bestV] = current.palette.getColor(oldColor);
            current.conflicts = conflicts;

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

            current.usedColors[oldColor]--;
            current.coloring[bestV] = current.palette.getColor(bestColor);
            current.usedColors[bestColor]++;
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
        greedyRemoveConflicts(current);
    }

    return current.coloring;
}

ColoringMap SimulatedAnnealingColoring::run(std::unique_ptr<StateNode> initialState, int iterations)
{
    auto current = std::move(*initialState); // Copy to stack by dereferencing
    std::cout << "Initial conflicts: " << current.conflicts << std::endl;

    for (int t = 1; t <= iterations; ++t)
    {
        double T = schedule_(t); // keep as double, don't truncate
        if (T <= 1e-12)
        { // stop when temperature effectively zero
            std::cout << "Temperature ~ 0, stopping.\n";
            break;
        }

        // 1) choose a vertex that contributes to conflicts (highest incident conflicts)
        auto bestV = selectNextNode(current);

        if (bestV == nullptr)
        {
            break;
        }

        int oldColorIdx = current.coloring[bestV].index;
        int selectedColorIdx = oldColorIdx;
        std::uniform_int_distribution<int> dist(0, static_cast<int>(current.palette.size() - 2));
        {
            int r = dist(rng_);
            if (r >= oldColorIdx)
                ++r;
            selectedColorIdx = r;
        }

        int oldInc = countNodeConflicts(bestV, current.coloring);
        Color savedOldColor = current.coloring[bestV];

        int oldConflicts = current.conflicts;
        int oldH = current.computeH();

        current.coloring[bestV] = current.palette.getColor(selectedColorIdx);
        int newInc = countNodeConflicts(bestV, current.coloring);
        int newConflicts = current.conflicts - oldInc + newInc;
        current.conflicts = newConflicts;
        current.usedColors[oldColorIdx]--;
        current.usedColors[selectedColorIdx]++;

        int newH = current.computeH();

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

        if (!accept)
        {
            current.coloring[bestV] = savedOldColor;
            current.conflicts = oldConflicts;
            current.usedColors[oldColorIdx]++;
            current.usedColors[selectedColorIdx]--;
        }
    }

    std::cout << "Final conflicts: " << current.conflicts << "\n";

    if (current.conflicts > 0)
    {
        std::cout << "Performing greedy conflict removal...\n";
        greedyRemoveConflicts(current);
    }

    return current.coloring;
}

double SimulatedAnnealingColoring::schedule_(int t)
{
    return 100.0 * std::pow(0.95, t);
}

ColoringMap BeamColoring::run(std::unique_ptr<StateNode> initialState, int iterations)
{
    auto start = std::move(*initialState); // Copy to stack by dereferencing
    std::cout << "Initial conflicts: " << start.conflicts << std::endl;
    k_ = (start.palette.size() - 1) / 2; // Set beam width to palette size - 1 for the color of current node

    std::vector<StateNode> beam;
    beam.reserve(k_);

    int paletteSize = start.palette.size(); // Store palette size before move
    beam.push_back(std::move(start));

    std::vector<StateNode> candidates;
    candidates.reserve(k_ * paletteSize);

    for (int it = 0; it < iterations; ++it)
    {
        for (auto &current : beam)
        {
            auto bestV = selectNextNode(current);
            if (!bestV)
                break;
            int oldColor = current.coloring[bestV].index;

            int oldInc = countNodeConflicts(bestV, current.coloring);
            int oldH = current.computeH();

            bool improved = false;
            std::shuffle(current.palette.begin(), current.palette.end(), rng_);

            for (int c = 0; c < k_; ++c)
            {
                if (c == oldColor)
                    c = c == (current.palette.size() - 1) ? k_ - 1 : k_ + 1;

                ColoringMap newColoring(current.coloring);
                newColoring[bestV] = current.palette.getColor(c);

                UsedColorsMap newUsedColors(current.usedColors);
                newUsedColors[oldColor]--;
                newUsedColors[c]++;

                int newInc = countNodeConflicts(bestV, newColoring);
                int newConflicts = current.conflicts - oldInc + newInc;

                candidates.emplace_back(
                    current.graph,
                    current.palette,
                    std::move(newColoring),
                    newConflicts,
                    c,
                    std::move(newUsedColors));
            }
        }
        beam = kLeast(candidates, k_);
        candidates.clear();
        for (int i = 0; i < (int)beam.size(); ++i)
        {
            if (beam[i].conflicts == 0)
            {
                std::cout << "Found conflict-free coloring in beam at iteration " << it << "\n";
                return beam[i].coloring;
            }
        }
    }

    StateNode result = std::move(beam[0]);
    std::cout << "Final conflicts: " << result.conflicts << "\n";

    if (result.conflicts > 0)
    {
        std::cout << "Performing greedy conflict removal...\n";
        greedyRemoveConflicts(result);
    }

    return result.coloring;
}

int partition(std::vector<StateNode> &arr, int left, int right)
{
    // Last element is chosen as a pivot.
    int pivotH = arr[right].computeH();
    int i = left;

    for (int j = left; j < right; j++)
    {
        if (arr[j].computeH() <= pivotH)
        {
            std::swap(arr[i], arr[j]);
            i++;
        }
    }

    std::swap(arr[i], arr[right]);

    return i;
}

void quickSelect(std::vector<StateNode> &arr, int left, int right, int k)
{
    if (left <= right)
    {
        int pivotIdx = partition(arr, left, right);

        // Count of all elements in the left part
        int leftCnt = pivotIdx - left + 1;

        // If leftCnt is equal to k, then we have
        // found the k largest element
        if (leftCnt == k)
            return;

        // Search in the left subarray
        if (leftCnt > k)
            quickSelect(arr, left, pivotIdx - 1, k);

        // Reduce the k by number of elements already covered
        // and search in the right subarray
        else
            quickSelect(arr, pivotIdx + 1, right, k - leftCnt);
    }
}

std::vector<StateNode> kLeast(std::vector<StateNode> &arr, int k)
{
    int n = arr.size();
    if (k > n)
        k = n;

    quickSelect(arr, 0, n - 1, k);

    // Move the first k elements to a new vector
    std::vector<StateNode> res;
    res.reserve(k);
    for (int i = 0; i < k; ++i)
    {
        res.push_back(std::move(arr[i]));
    }
    return res;
}