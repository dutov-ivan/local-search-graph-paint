#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <random>

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

private:
    Color generateColor(int i) const
    {
        return Color{i, (i * 97) % 256, (i * 57) % 256, (i * 37) % 256};
    }

    int presetCount_;
    std::vector<Color> presetColors_;
};

// Forward declarations
int computeNodeSaturation(Node *node, const ColoringMap &coloring);
ColoringMap greedyColoring(Graph &graph);
std::unordered_map<Node *, int> computeSaturation(Graph &graph, const ColoringMap &coloring);

struct Algorithm
{
    virtual ~Algorithm() = default;
    virtual std::unordered_map<Node *, Color> run(Graph &graph, int iterations) = 0;
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

    std::unordered_map<Node *, Color> run(Graph &graph, int iterations) override;

private:
    std::mt19937 rng_;
};

#endif // ALGORITHM_H