// Entry point: generate a random graph, color it, and export an image
#include <iostream>
#include <string>
#include "graph.h"
#include "algorithms.h"
#include "visualization.h"

int main()
{
    try
    {
        Graph graph;
        // Use moderate defaults to keep runtime and visualization reasonable
        const std::size_t numVertices = 10;
        const std::size_t numEdges = 20;
        graph.generateRandomGraph(numVertices, numEdges, /*allowSelfLoops=*/false, /*seed=*/0);

        HillClimbingColoring solver;
        const int iterations = 1000;
        ColoringMap coloring = solver.run(graph, iterations);

        // Count distinct colors
        std::unordered_set<int> distinctColors;
        for (const auto &[node, color] : coloring)
        {
            distinctColors.insert(color.index);
        }
        std::cout << "Used " << distinctColors.size() << " colors." << std::endl;

        const std::string dotPath = "graph.dot";
        const std::string imgPath = "graph.png";

        // Avoid rendering extremely large graphs which can fail or take too long
        if (graph.getNodes().size() <= 2000)
        {
            if (!visualizeGraph(graph, &coloring, dotPath, imgPath))
            {
                std::cerr << "Failed to visualize graph. Ensure Graphviz is installed and 'dot' is in PATH." << std::endl;
                return 1;
            }
        }

        std::cout << "Generated random graph with " << numVertices << " vertices and " << numEdges
                  << " edges. Wrote " << dotPath << " and " << imgPath << std::endl;
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
}
