#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

struct Color
{
    int order;
    int r, g, b;
};

typedef std::unordered_map<Node *, Color> ColoringMap;

// Hardcoded set of 16 clearly discernable colors
static const Color PRESET_COLORS[16] = {
    {0, 255, 0, 0},      // Red
    {1, 0, 255, 0},      // Green
    {2, 0, 0, 255},      // Blue
    {3, 255, 255, 0},    // Yellow
    {4, 255, 0, 255},    // Magenta
    {5, 0, 255, 255},    // Cyan
    {6, 255, 128, 0},    // Orange
    {7, 128, 0, 255},    // Purple
    {8, 128, 255, 0},    // Lime
    {9, 0, 128, 255},    // Sky Blue
    {10, 255, 0, 128},   // Pink
    {11, 128, 255, 255}, // Light Cyan
    {12, 255, 255, 128}, // Light Yellow
    {13, 128, 128, 128}, // Gray
    {14, 255, 128, 255}, // Light Magenta
    {15, 128, 255, 128}  // Mint
};

inline int getPresetColorCount() { return 16; }
inline Color getPresetColor(int idx) { return PRESET_COLORS[idx % getPresetColorCount()]; }

// Forward declarations
int computeNodeSaturation(Node *node, const ColoringMap &coloring);
ColoringMap greedyColoring(Graph &graph, int minColors);
std::unordered_map<Node *, int> computeSaturation(Graph &graph, const ColoringMap &coloring);

struct Algorithm
{
    virtual ~Algorithm() = default;
    virtual std::unordered_map<Node *, Color> run(Graph &graph, int iterations) = 0;
};

class HillClimbingColoring : public Algorithm
{
public:
    HillClimbingColoring() = default;
    std::unordered_map<Node *, Color> run(Graph &graph, int iterations) override;
};

#endif // ALGORITHM_H