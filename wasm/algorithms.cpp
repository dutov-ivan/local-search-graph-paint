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

static int countNodeConflicts(std::shared_ptr<GraphNode> node, const ColoringMap &coloring)
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
    for (auto node : state.graph->getNodes())
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
    for (auto v : state.graph->getNodes())
    {
        auto itv = state.coloring.find(v);
        if (itv == state.coloring.end())
            continue;
        int vcol = itv->second.index;

        // how many neighbors share v's color
        int incident = 0;
        for (auto nbr : v->getNeighbors())
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

HillClimbingColoringIterator::HillClimbingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng)
    : current_(std::move(*initialState)), maxIterations_(maxIterations), iteration_(0), finished_(false), rng_(std::move(rng)), greedyDone_(false)
{
}

StateNode HillClimbingColoringIterator::step()
{
    if (iteration_ >= maxIterations_)
    {
        current_.continueIteration = false;
        return current_;
    }

    auto bestV = selectNextNode(current_);
    if (!bestV)
    {
        current_.continueIteration = false;
        return current_;
    }

    int oldColor = current_.coloring[bestV].index;
    int oldH = current_.computeH();
    int bestH = oldH;
    int bestColor = oldColor;
    bool improved = false;
    for (int c = 0; c < (int)current_.palette.size(); ++c)
    {
        if (c == oldColor)
            continue;
        StateNode snapshot(current_.graph, current_.palette, current_.coloring, current_.conflicts, current_.usedColors);
        snapshot.forward(current_.palette.getColor(c), bestV);
        int newH = snapshot.computeH();
        if (newH < bestH)
        {
            bestH = newH;
            bestColor = c;
            improved = true;
        }
    }
    if (improved)
    {
        current_.forward(current_.palette.getColor(bestColor), bestV);
    }
    else
    {
        finished_ = true;
        current_.continueIteration = false;
    }
    iteration_++;
    current_.node = bestV;
    current_.color = current_.palette.getColor(bestColor);
    current_.continueIteration = !finished_;
    return current_;
}

const ColoringMap &HillClimbingColoringIterator::getColoring() const { return current_.coloring; }
const StateNode &HillClimbingColoringIterator::getState() const { return current_; }

SimulatedAnnealingColoringIterator::SimulatedAnnealingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng)
    : current_(std::move(*initialState)), maxIterations_(maxIterations), iteration_(1), finished_(false), rng_(std::move(rng)), greedyDone_(false)
{
}

StateNode SimulatedAnnealingColoringIterator::step()
{
    if (finished_)
    {
        current_.continueIteration = false;
        return current_;
    }
    if (iteration_ > maxIterations_)
    {
        finished_ = true;
    }
    double T = schedule_(iteration_);
    if (T <= 1e-12)
    {
        std::cout << "Temperature ~ 0, stopping.\n";
        finished_ = true;
    }
    if (finished_)
    {
        current_.continueIteration = false;
        return current_;
    }
    auto bestV = selectNextNode(current_);
    if (!bestV)
    {
        finished_ = true;
        current_.continueIteration = false;
        return current_;
    }
    int oldColorIdx = current_.coloring[bestV].index;
    int selectedColorIdx = oldColorIdx;
    std::uniform_int_distribution<int> dist(0, static_cast<int>(current_.palette.size() - 2));
    int r = dist(rng_);
    if (r >= oldColorIdx)
        ++r;
    selectedColorIdx = r;
    int oldInc = countNodeConflicts(bestV, current_.coloring);
    Color savedOldColor = current_.coloring[bestV];
    int oldConflicts = current_.conflicts;
    int oldH = current_.computeH();
    current_.coloring[bestV] = current_.palette.getColor(selectedColorIdx);
    int newInc = countNodeConflicts(bestV, current_.coloring);
    int newConflicts = current_.conflicts - oldInc + newInc;
    current_.conflicts = newConflicts;
    current_.usedColors[oldColorIdx]--;
    current_.usedColors[selectedColorIdx]++;
    int newH = current_.computeH();
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
        current_.coloring[bestV] = savedOldColor;
        current_.conflicts = oldConflicts;
        current_.usedColors[selectedColorIdx]--;
        current_.usedColors[oldColorIdx]++;
    }
    iteration_++;
    current_.node = bestV;
    current_.color = current_.palette.getColor(accept ? selectedColorIdx : oldColorIdx);
    current_.continueIteration = !finished_;
    return current_;
}

const ColoringMap &SimulatedAnnealingColoringIterator::getColoring() const { return current_.coloring; }
const StateNode &SimulatedAnnealingColoringIterator::getState() const { return current_; }
double SimulatedAnnealingColoringIterator::schedule_(int t) { return 100.0 * std::pow(0.95, t); }

BeamColoringIterator::BeamColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng)
    : k_(0), paletteSize_(0), maxIterations_(maxIterations), iteration_(0), finished_(false), rng_(std::move(rng)), greedyDone_(false)
{
    StateNode start = std::move(*initialState);
    std::cout << "Initial conflicts: " << start.conflicts << std::endl;
    k_ = (start.palette.size() - 1) / 2;
    paletteSize_ = start.palette.size();
    beam_.reserve(k_);
    beam_.push_back(std::move(start));
    candidates_.reserve(k_ * paletteSize_);
}

StateNode BeamColoringIterator::step()
{
    if (finished_)
    {
        if (!beam_.empty())
            beam_[0].continueIteration = false;
        return beam_.empty() ? StateNode() : beam_[0];
    }
    if (iteration_ >= maxIterations_)
    {
        finished_ = true;
    }
    if (finished_)
    {
        if (!beam_.empty())
            beam_[0].continueIteration = false;
        return beam_.empty() ? StateNode() : beam_[0];
    }
    for (auto &current : beam_)
    {
        auto bestV = selectNextNode(current);
        if (!bestV)
            continue;
        int oldColor = current.coloring[bestV].index;
        int oldInc = countNodeConflicts(bestV, current.coloring);
        int oldH = current.computeH();
        std::shuffle(current.palette.begin(), current.palette.end(), rng_);
        for (int c = 0; c < k_; ++c)
        {
            if (c == oldColor)
                continue;
            ColoringMap newColoring(current.coloring);
            newColoring[bestV] = current.palette.getColor(c);
            UsedColorsMap newUsedColors(current.usedColors);
            newUsedColors[oldColor]--;
            newUsedColors[c]++;
            int newInc = countNodeConflicts(bestV, newColoring);
            int newConflicts = current.conflicts - oldInc + newInc;
            StateNode newState(
                current.graph,
                current.palette,
                std::move(newColoring),
                newConflicts,
                std::move(newUsedColors));
            newState.node = bestV;
            newState.color = current.palette.getColor(c);
            newState.conflicts = newConflicts;
            newState.continueIteration = true;
            candidates_.push_back(std::move(newState));
        }
    }
    beam_ = kLeast(candidates_, k_);
    candidates_.clear();
    for (int i = 0; i < (int)beam_.size(); ++i)
    {
        if (beam_[i].conflicts == 0)
        {
            std::cout << "Found conflict-free coloring in beam at iteration " << iteration_ << "\n";
            finished_ = true;
            break;
        }
    }
    iteration_++;
    if (finished_ && !beam_.empty())
    {
        std::cout << "Final conflicts: " << beam_[0].conflicts << "\n";
    }
    if (!beam_.empty())
        beam_[0].continueIteration = !finished_;
    return beam_.empty() ? StateNode() : beam_[0];
}

const ColoringMap &BeamColoringIterator::getColoring() const
{
    if (beam_.empty())
        throw std::runtime_error("Beam is empty");
    return beam_[0].coloring;
}
const StateNode &BeamColoringIterator::getState() const
{
    if (beam_.empty())
        throw std::runtime_error("Beam is empty");
    return beam_[0];
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