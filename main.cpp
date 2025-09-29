// Entry point: generate a random graph, color it, and compare different algorithms
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include "graph.h"
#include "algorithms.h"
#include "visualization.h"

int countDistinctColors(const ColoringMap &coloring)
{
    std::unordered_set<int> distinctColors;
    for (const auto &[node, color] : coloring)
    {
        distinctColors.insert(color.index);
    }
    return static_cast<int>(distinctColors.size());
}

int main()
{
    try
    {
        // Get input parameters
        std::size_t numVertices, numEdges;
        std::cout << "Enter number of vertices: ";
        std::cin >> numVertices;
        std::cout << "Enter number of edges: ";
        std::cin >> numEdges;

        // Generate the same random graph for all algorithms
        auto graph = std::make_shared<Graph>();
        const int seed = 42; // Fixed seed for reproducible results
        graph->generateRandomGraph(numVertices, numEdges, /*allowSelfLoops=*/false, seed);

        const int iterations = 1000; // Same iteration count for all algorithms

        std::cout << "\n========================================" << std::endl;
        std::cout << "Graph Coloring Algorithm Comparison" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Graph: " << numVertices << " vertices, " << numEdges << " edges" << std::endl;
        std::cout << "Iterations: " << iterations << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "========================================\n"
                  << std::endl;

        // Algorithm 1: Hill Climbing
        std::cout << "1. HILL CLIMBING ALGORITHM" << std::endl;
        std::cout << "----------------------------" << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();

        HillClimbingColoring hillClimbingSolver;
        ColoringMap hillClimbingColoring = hillClimbingSolver.run(graph, iterations);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        int hillClimbingColors = countDistinctColors(hillClimbingColoring);
        std::cout << "Colors used: " << hillClimbingColors << std::endl;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

        // Save visualization for hill climbing
        if (isVisualization(numVertices, numEdges))
        {
            if (visualizeGraph(graph, &hillClimbingColoring, "hill_climbing.dot", "hill_climbing.png"))
            {
                std::cout << "Visualization saved: hill_climbing.dot, hill_climbing.png" << std::endl;
            }
            else
            {
                std::cout << "Failed to create visualization" << std::endl;
            }
        }
        else
        {
            std::cout << "Graph too large for visualization (> 100 vertices)" << std::endl;
        }
        std::cout << std::endl;

        // Algorithm 2: Simulated Annealing
        std::cout << "2. SIMULATED ANNEALING ALGORITHM" << std::endl;
        std::cout << "---------------------------------" << std::endl;
        start_time = std::chrono::high_resolution_clock::now();

        SimulatedAnnealingColoring simulatedAnnealingSolver;
        ColoringMap simulatedAnnealingColoring = simulatedAnnealingSolver.run(graph, iterations);

        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        int simulatedAnnealingColors = countDistinctColors(simulatedAnnealingColoring);
        std::cout << "Colors used: " << simulatedAnnealingColors << std::endl;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

        // Save visualization for simulated annealing
        if (isVisualization(numVertices, numEdges))
        {
            if (visualizeGraph(graph, &simulatedAnnealingColoring, "simulated_annealing.dot", "simulated_annealing.png"))
            {
                std::cout << "Visualization saved: simulated_annealing.dot, simulated_annealing.png" << std::endl;
            }
            else
            {
                std::cout << "Failed to create visualization" << std::endl;
            }
        }
        else
        {
            std::cout << "Graph too large for visualization (> 100 vertices)" << std::endl;
        }
        std::cout << std::endl;

        // Algorithm 3: Beam Search
        std::cout << "3. BEAM SEARCH ALGORITHM" << std::endl;
        std::cout << "------------------------" << std::endl;
        start_time = std::chrono::high_resolution_clock::now();

        BeamColoring beamSolver;
        ColoringMap beamColoring = beamSolver.run(graph, iterations);

        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        int beamColors = countDistinctColors(beamColoring);
        std::cout << "Colors used: " << beamColors << std::endl;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

        // Save visualization for beam search
        if (isVisualization(numVertices, numEdges))
        {
            if (visualizeGraph(graph, &beamColoring, "beam_search.dot", "beam_search.png"))
            {
                std::cout << "Visualization saved: beam_search.dot, beam_search.png" << std::endl;
            }
            else
            {
                std::cout << "Failed to create visualization" << std::endl;
            }
        }
        else
        {
            std::cout << "Graph too large for visualization (> 100 vertices)" << std::endl;
        }
        std::cout << std::endl;

        // Summary
        std::cout << "========================================" << std::endl;
        std::cout << "COMPARISON SUMMARY" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::left << std::setw(25) << "Algorithm" << std::setw(15) << "Colors Used" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << std::left << std::setw(25) << "Hill Climbing" << std::setw(15) << hillClimbingColors << std::endl;
        std::cout << std::left << std::setw(25) << "Simulated Annealing" << std::setw(15) << simulatedAnnealingColors << std::endl;
        std::cout << std::left << std::setw(25) << "Beam Search" << std::setw(15) << beamColors << std::endl;
        std::cout << "========================================" << std::endl;

        // Find best algorithm
        int bestColors = std::min({hillClimbingColors, simulatedAnnealingColors, beamColors});
        std::cout << "Best result: " << bestColors << " colors ";
        if (hillClimbingColors == bestColors)
            std::cout << "(Hill Climbing)";
        if (simulatedAnnealingColors == bestColors)
            std::cout << "(Simulated Annealing)";
        if (beamColors == bestColors)
            std::cout << "(Beam Search)";
        std::cout << std::endl;

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
}
