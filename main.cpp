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
        const std::size_t numVertices = 10000;
        const std::size_t numEdges = 400000;
        graph.generateRandomGraph(numVertices, numEdges, /*allowSelfLoops=*/false, /*seed=*/0);

        HillClimbingColoring solver;
        const int iterations = 10;
        ColoringMap coloring = solver.run(graph, iterations);

        const std::string dotPath = "graph.dot";
        const std::string imgPath = "graph.png";

        if (!visualizeGraph(graph, &coloring, dotPath, imgPath, "dot", "png", /*labelWithColorOrder=*/true))
        {
            std::cerr << "Failed to visualize graph. Ensure Graphviz is installed and 'dot' is in PATH." << std::endl;
            return 1;
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
