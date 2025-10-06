#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <map> // For embind-friendly map bindings
#include <algorithm>
#include <iostream>
#include <random>
#include <functional>
#include <vector>
#include <memory>
#include <emscripten/bind.h>

struct Color
{
    int index;
    int r, g, b;
};

typedef std::map<std::shared_ptr<GraphNode>, Color> ColoringMap;

class ColorPalette
{
public:
    ColorPalette(int presetCount)
        : presetCount_(presetCount)
    {
        for (int i = 0; i < presetCount_; ++i)
        {
            presetColors_.push_back(generateColor(i));
        }
    }

    const Color &getColor(int i) const
    {
        return presetColors_[i];
    }

    void addColor()
    {
        presetColors_.push_back(generateColor(presetCount_++));
    }

    int size() const
    {
        return presetCount_;
    }

    const std::vector<Color> &getColors() const { return presetColors_; }

    auto begin() const { return presetColors_.begin(); }
    auto end() const { return presetColors_.end(); }

    auto begin() { return presetColors_.begin(); }
    auto end() { return presetColors_.end(); }

private:
    Color generateColor(int i) const
    {
        return Color{i, (i * 97) % 256, (i * 57) % 256, (i * 37) % 256};
    }

    int presetCount_;
    std::vector<Color> presetColors_;
};

// Same rationale as above: switch to std::map for embind compatibility.
typedef std::map<int, int> UsedColorsMap;

struct StepResult
{
    std::shared_ptr<GraphNode> node;
    Color color;
    int conflicts;
    bool continueIteration;

    StepResult(int conflicts = 0)
        : node(nullptr), color(), conflicts(conflicts), continueIteration(false)
    {
    }

    StepResult(bool continueIter)
        : node(nullptr), color(), conflicts(0), continueIteration(continueIter)
    {
    }

    StepResult(std::shared_ptr<GraphNode> n, Color c, int conf, bool cont)
        : node(std::move(n)), color(c), conflicts(conf), continueIteration(cont)
    {
    }
};

struct StateNode
{
public:
    int computeH() const
    {
        auto it = usedColors.find(color.index);
        int colorUsage = (it != usedColors.end()) ? it->second : 0;
        return conflicts * 100 - colorUsage;
    }

    void forward(Color c, std::shared_ptr<GraphNode> node)
    {
        // Efficiently update conflicts using oldInc/newInc trick
        int oldColorIdx = coloring.at(node).index;
        int oldInc = 0, newInc = 0;
        for (const auto &nbr : node->getNeighbors())
        {
            auto it = coloring.find(nbr);
            if (it != coloring.end())
            {
                if (it->second.index == oldColorIdx)
                    oldInc++;
                if (it->second.index == c.index)
                    newInc++;
            }
        }
        // Update coloring and usedColors
        coloring[node] = c;
        usedColors[oldColorIdx]--;
        usedColors[c.index]++;
        // Update conflicts
        int newConflicts = conflicts - oldInc + newInc;
        this->node = node;
        color = c;
        conflicts = newConflicts;
        continueIteration = true;
    }

    StateNode()
        : graph(nullptr),
          palette(0), // Initialize with 0 preset colors
          coloring(),
          usedColors(),
          node(nullptr),
          color(),
          conflicts(0),
          continueIteration(false)
    {
    }

    StateNode(std::shared_ptr<Graph> g, ColorPalette p, ColoringMap c, int conflicts, UsedColorsMap used)
        : graph(std::move(g)),
          palette(std::move(p)),
          coloring(std::move(c)),
          usedColors(std::move(used)),
          node(nullptr),
          color(),
          conflicts(conflicts),
          continueIteration(false)
    {
    }

    // default constructor, destructor ok
    StateNode(StateNode &&other) noexcept
        : graph(std::move(other.graph)),
          palette(std::move(other.palette)),
          coloring(std::move(other.coloring)),
          usedColors(std::move(other.usedColors)),
          node(std::move(other.node)),
          color(other.color),
          conflicts(other.conflicts),
          continueIteration(other.continueIteration) {}

    StateNode(const StateNode &other)
        : graph(other.graph),
          palette(other.palette),
          coloring(other.coloring),
          usedColors(other.usedColors),
          node(other.node),
          color(other.color),
          conflicts(other.conflicts),
          continueIteration(other.continueIteration) {}

    StateNode &operator=(const StateNode &other)
    {
        if (this != &other)
        {
            graph = other.graph;
            palette = other.palette;
            coloring = other.coloring;
            usedColors = other.usedColors;
            node = other.node;
            color = other.color;
            conflicts = other.conflicts;
            continueIteration = other.continueIteration;
        }
        return *this;
    }

    StateNode &operator=(StateNode &&other) noexcept
    {
        if (this != &other)
        {
            graph = std::move(other.graph);
            palette = std::move(other.palette);
            coloring = std::move(other.coloring);
            usedColors = std::move(other.usedColors);
            node = std::move(other.node);
            color = other.color;
            conflicts = other.conflicts;
            continueIteration = other.continueIteration;
        }
        return *this;
    }

    std::shared_ptr<Graph> graph;
    ColorPalette palette;
    ColoringMap coloring;
    UsedColorsMap usedColors;
    // Flattened former StepResult data:
    std::shared_ptr<GraphNode> node; // last modified / selected node
    Color color;                     // color applied in last step
    int conflicts;                   // current number of conflicts
    bool continueIteration;          // whether algorithm can continue
};

struct AlgorithmIterator
{
    virtual ~AlgorithmIterator() = default;
    // Step one iteration, returns tuple (node, color, continue)
    virtual StateNode step() = 0; // now returns the whole (copied) state
    // Step until done
    virtual void runToEnd()
    {
        while (step().continueIteration)
            ;
    }
    // Get current coloring
    virtual const ColoringMap &getColoring() const = 0;
    // Get current state
    virtual const StateNode &getState() const = 0;
    // Expose current iteration counter
    virtual int currentIteration() const = 0;
};

class HillClimbingColoringIterator : public AlgorithmIterator
{
public:
    HillClimbingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng = std::mt19937(std::random_device{}()));
    StateNode step() override;
    void runToEnd() override
    {
        while (step().continueIteration)
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;
    int currentIteration() const override { return iteration_; }

private:
    StateNode current_;
    int maxIterations_;
    int iteration_;
    bool finished_;
    std::mt19937 rng_;
    bool greedyDone_;
};

class SimulatedAnnealingColoringIterator : public AlgorithmIterator
{
public:
    SimulatedAnnealingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng = std::mt19937(std::random_device{}()));
    StateNode step() override;
    void runToEnd() override
    {
        while (step().continueIteration)
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;
    int currentIteration() const override { return iteration_; }

private:
    StateNode current_;
    int maxIterations_;
    int iteration_;
    bool finished_;
    std::mt19937 rng_;
    bool greedyDone_;
    double schedule_(int t);
};

class BeamColoringIterator : public AlgorithmIterator
{
public:
    BeamColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng = std::mt19937(std::random_device{}()));
    StateNode step() override;
    void runToEnd() override
    {
        while (step().continueIteration)
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;
    int currentIteration() const override { return iteration_; }

private:
    std::vector<StateNode> beam_;
    std::vector<StateNode> candidates_;
    int k_;
    int paletteSize_;
    int maxIterations_;
    int iteration_;
    bool finished_;
    std::mt19937 rng_;
    bool greedyDone_;
};

std::vector<StateNode> kLeast(std::vector<StateNode> &arr, int k);

int computeConflicts(const Graph &graph, const ColoringMap &coloring);
void greedyRemoveConflicts(StateNode &state);

#endif // ALGORITHM_H