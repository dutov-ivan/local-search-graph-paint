#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <random>
#include <functional>
#include <vector>
#include <memory>

struct Color
{
    int index;
    int r, g, b;
};

typedef std::unordered_map<Node *, Color> ColoringMap;

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

typedef std::unordered_map<int, int> UsedColorsMap;

struct StateNode
{
public:
    int computeH() const
    {
        auto it = usedColors.find(lastUsedColorIndex);
        int colorUsage = (it != usedColors.end()) ? it->second : 0;
        return conflicts * 100 + colorUsage;
    }

    StateNode() = default;

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

struct Algorithm
{
    virtual ~Algorithm() = default;
    virtual std::unordered_map<Node *, Color> run(std::shared_ptr<Graph> graph, int iterations) = 0;
};

class HillClimbingColoring : public Algorithm
{
public:
    // Default ctor seeds RNG using std::random_device (no high_resolution_clock)
    HillClimbingColoring()
        : rng_(std::mt19937(std::random_device{}())) {}

    // Prefer passing an RNG so the caller controls determinism/seeding
    explicit HillClimbingColoring(std::mt19937 rng)
        : rng_(std::move(rng)) {}

    std::unordered_map<Node *, Color> run(std::shared_ptr<Graph> graph, int iterations) override;

private:
    std::mt19937 rng_;
};

class SimulatedAnnealingColoring : public Algorithm
{
public:
    SimulatedAnnealingColoring()
        : rng_(std::mt19937(std::random_device{}())) {}

    explicit SimulatedAnnealingColoring(std::mt19937 rng)
        : rng_(std::move(rng)) {}

    std::unordered_map<Node *, Color> run(std::shared_ptr<Graph> graph, int iterations) override;

private:
    std::mt19937 rng_;
    double schedule_(int t);
};

class BeamColoring : public Algorithm
{
public:
    BeamColoring()
        : rng_(std::mt19937(std::random_device{}())) {}

    explicit BeamColoring(std::mt19937 rng)
        : rng_(std::move(rng)) {}

    std::unordered_map<Node *, Color> run(std::shared_ptr<Graph> graph, int iterations) override;

private:
    int k_;
    std::mt19937 rng_;
};

std::vector<StateNode> kLeast(std::vector<StateNode> &arr, int k);

#endif // ALGORITHM_H