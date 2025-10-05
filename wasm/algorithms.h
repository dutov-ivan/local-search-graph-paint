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

struct StateNode
{
public:
    int computeH() const
    {
        auto it = usedColors.find(lastUsedColorIndex);
        int colorUsage = (it != usedColors.end()) ? it->second : 0;
        return conflicts * 100 - colorUsage;
    }

    StateNode()
        : graph(nullptr),
          palette(0), // Initialize with 0 preset colors
          coloring(),
          conflicts(0),
          lastUsedColorIndex(0),
          usedColors()
    {
    }

    // Parameterized constructor
    StateNode(std::shared_ptr<Graph> g, ColorPalette p, ColoringMap c, int conf, int lastColor, UsedColorsMap used)
        : graph(std::move(g)), palette(std::move(p)), coloring(std::move(c)),
          conflicts(conf), lastUsedColorIndex(lastColor), usedColors(std::move(used)) {}

    // default constructor, destructor ok
    StateNode(StateNode &&other) noexcept
        : graph(std::move(other.graph)),
          palette(std::move(other.palette)),
          coloring(std::move(other.coloring)),
          conflicts(other.conflicts),
          lastUsedColorIndex(other.lastUsedColorIndex),
          usedColors(std::move(other.usedColors)) {}

    StateNode &operator=(StateNode &&other) noexcept
    {
        if (this != &other)
        {
            graph = std::move(other.graph);
            palette = std::move(other.palette);
            coloring = std::move(other.coloring);
            conflicts = other.conflicts;
            lastUsedColorIndex = other.lastUsedColorIndex;
            usedColors = std::move(other.usedColors);
        }
        return *this;
    }

    StateNode(const StateNode &) = delete; // disable copy if necessary
    StateNode &operator=(const StateNode &) = delete;

    std::shared_ptr<Graph> graph;
    ColorPalette palette;
    ColoringMap coloring;
    int conflicts;
    int lastUsedColorIndex;
    UsedColorsMap usedColors;
};

struct AlgorithmIterator
{
    virtual ~AlgorithmIterator() = default;
    // Step one iteration, returns true if can continue, false if done
    virtual bool step() = 0;
    // Step until done
    virtual void runToEnd()
    {
        while (step())
            ;
    }
    // Get current coloring
    virtual const ColoringMap &getColoring() const = 0;
    // Get current state
    virtual const StateNode &getState() const = 0;
};

class HillClimbingColoringIterator : public AlgorithmIterator
{
public:
    HillClimbingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng = std::mt19937(std::random_device{}()));
    bool step() override;
    void runToEnd() override
    {
        while (step())
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;

private:
    StateNode current_;
    int maxIterations_;
    int iteration_;
    int conflicts_;
    bool finished_;
    std::mt19937 rng_;
    bool greedyDone_;
};

class SimulatedAnnealingColoringIterator : public AlgorithmIterator
{
public:
    SimulatedAnnealingColoringIterator(std::unique_ptr<StateNode> initialState, int maxIterations, std::mt19937 rng = std::mt19937(std::random_device{}()));
    bool step() override;
    void runToEnd() override
    {
        while (step())
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;

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
    bool step() override;
    void runToEnd() override
    {
        while (step())
            ;
    }
    const ColoringMap &getColoring() const override;
    const StateNode &getState() const override;

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

#endif // ALGORITHM_H