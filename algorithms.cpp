// Algorithm implementations moved from header to avoid multiple definition issues
#include "algorithms.h"
#include "visualization.h"

#include <vector>

namespace
{
    Color generateExtraColor(int order)
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
            neighborColors.insert(it->second.order);
        }
    }
    return static_cast<int>(neighborColors.size());
}

// Simple greedy coloring using ColorGenerator
ColoringMap greedyColoring(Graph &graph)
{
    ColoringMap coloring;
    const int presetCount = getPresetColorCount();
    const int paletteLimit = presetCount;
    int nextDynamicOrder = presetCount;

    for (auto &node : graph.getNodes())
    {
        std::unordered_set<int> neighborColors;
        for (auto &neighbor : node->getNeighbors())
        {
            auto it = coloring.find(neighbor);
            if (it != coloring.end())
            {
                neighborColors.insert(it->second.order);
            }
        }

        Color color;
        bool found = false;
        // Try to use colors within the requested palette first
        for (int i = 0; i < paletteLimit; ++i)
        {
            Color candidate = getPresetColor(i);
            if (neighborColors.count(candidate.order) == 0)
            {
                color = candidate;
                found = true;
                break;
            }
        }

        // If that fails, allow any remaining preset colors
        if (!found)
        {
            for (int i = paletteLimit; i < presetCount; ++i)
            {
                Color candidate = getPresetColor(i);
                if (neighborColors.count(candidate.order) == 0)
                {
                    color = candidate;
                    found = true;
                    break;
                }
            }
        }

        // As a last resort, synthesize a unique color
        if (!found)
        {
            const int order = nextDynamicOrder++;
            color = generateExtraColor(order);
        }

        coloring[node] = color;
    }

    return coloring;
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

std::unordered_map<Node *, Color> HillClimbingColoring::run(Graph &graph, int iterations)
{
    // Initialize coloring with greedy algorithm
    ColoringMap coloring = greedyColoring(graph);

    // Print number of different colors after greedy coloring
    {
        std::unordered_set<int> usedColors;
        for (const auto &pair : coloring)
        {
            usedColors.insert(pair.second.order);
        }
        std::cout << "Colors after greedy coloring: " << usedColors.size() << std::endl;
    }
    // Save initial coloring for debugging only if graph size <= 1000
    if (graph.getNodes().size() <= 1000)
    {
        visualizeGraph(graph, &coloring, "initial_coloring.dot", "initial_coloring.png", "dot", "png", true);
    }

    auto saturation = computeSaturation(graph, coloring);
    int overallSaturation = 0;
    for (const auto &pair : saturation)
    {
        overallSaturation += pair.second;
    }

    // Get another search node by getting nodes with highest saturation (or secondary by degree) and changing their color
    for (int iter = 0; iter < iterations; ++iter)
    {
        // Find the node with the highest saturation
        Node *targetNode = nullptr;
        int maxSaturation = -1;
        for (const auto &pair : saturation)
        {
            if (pair.second > maxSaturation)
            {
                maxSaturation = pair.second;
                targetNode = pair.first;
            }
            else if (pair.second == maxSaturation && pair.first->getNeighbors().size() > (targetNode ? targetNode->getNeighbors().size() : 0))
            {
                targetNode = pair.first; // Secondary criterion: degree
            }
        }

        if (!targetNode)
            break; // No nodes to process

        // Try changing the color of the target node to a different color
        std::unordered_set<int> neighborColors;
        for (auto &neighbor : targetNode->getNeighbors())
        {
            if (coloring.find(neighbor) != coloring.end())
            {
                neighborColors.insert(coloring[neighbor].order);
            }
        }

        // Try all available new colors for the target node
        int initialNeighborSaturation = 0;
        for (auto &neighbor : targetNode->getNeighbors())
        {
            initialNeighborSaturation += saturation[neighbor];
        }

        int bestNeighborSaturation = initialNeighborSaturation;
        Color originalColor = coloring[targetNode];
        Color bestColor = originalColor;
        bool foundBetter = false;

        std::vector<Color> candidateColors;
        for (auto &neighbor : targetNode->getNeighbors())
        {
            if (coloring.find(neighbor) != coloring.end())
            {
                candidateColors.push_back(coloring[neighbor]);
            }
        }

        for (const Color &candidateColor : candidateColors)
        {
            if (candidateColor.order == originalColor.order)
                continue;
            if (neighborColors.count(candidateColor.order) != 0)
                continue;

            // Temporarily assign the candidate color
            coloring[targetNode] = candidateColor;

            // Recompute saturation for affected neighbors
            int tempNeighborSaturation = 0;
            for (auto &neighbor : targetNode->getNeighbors())
            {
                tempNeighborSaturation += computeNodeSaturation(neighbor, coloring);
            }

            coloring[targetNode] = originalColor;

            if (tempNeighborSaturation < bestNeighborSaturation)
            {
                bestNeighborSaturation = tempNeighborSaturation;
                bestColor = candidateColor;
                foundBetter = true;
            }
        }

        // Restore the best coloring found
        if (foundBetter)
        {
            coloring[targetNode] = bestColor;
            // Update saturation for affected nodes
            for (auto &neighbor : targetNode->getNeighbors())
            {
                saturation[neighbor] = computeNodeSaturation(neighbor, coloring);
            }
            // Target node saturation won't change as its neighbors' colors didn't change
            overallSaturation = bestNeighborSaturation;
        }
        else
        {
            coloring[targetNode] = originalColor;
            break; // No improvement found, exit early
        }
    }

    // Print number of different colors after hill climbing
    {
        std::unordered_set<int> usedColors;
        for (const auto &pair : coloring)
        {
            usedColors.insert(pair.second.order);
        }
        std::cout << "Colors after hill climbing: " << usedColors.size() << std::endl;
    }
    return coloring;
}
