
#include <emscripten/bind.h>
#include "graph.h"
#include "algorithms.h"
#include <memory>
#include <optional>
#include "init.h"
using namespace emscripten;

struct AlgorithmStartupOptions
{
    std::string algorithmName = "hill_climbing";
    int iterations = 0;
    RandomGraphOptions generationOptions;
};

StateNode initialStateNode(const RandomGraphOptions &options, std::mt19937 &rng_)
{
    auto graph = std::make_shared<Graph>(generateRandomGraph(options, rng_));
    const auto &nodes = graph->getNodes();

    // compute maxDegree once
    int maxDegree = 0;
    for (auto n : nodes)
        maxDegree = std::max<int>(maxDegree, n->getNeighbors().size());

    ColorPalette palette(maxDegree + 1); // enough colors for any node's incident edges

    ColoringMap coloring;
    std::map<int, int> usedColors;
    std::uniform_int_distribution<int> colorDist(0, static_cast<int>(palette.size()) - 1);
    for (auto node : nodes)
    {
        coloring[node] = palette.getColor(colorDist(rng_));
        usedColors[coloring[node].index]++;
    }

    int conflicts = computeConflicts(*graph, coloring);
    return StateNode{graph, std::move(palette), std::move(coloring), conflicts, 0, std::move(usedColors)};
}

std::unique_ptr<Algorithm> getAlgorithm(const std::string &algorithmName)
{
    if (algorithmName == "hill_climbing")
    {
        return std::make_unique<HillClimbingColoring>(init.getRng());
    }
    else if (algorithmName == "simulated_annealing")
    {
        return std::make_unique<SimulatedAnnealingColoring>(init.getRng());
    }
    else if (algorithmName == "beam")
    {
        return std::make_unique<BeamColoring>(init.getRng());
    }
    else
    {
        throw std::invalid_argument("Unknown algorithm name: " + algorithmName);
    }
}

// Binding: Generate and set initialStateNode in global state, return it
void setInitialAlgorithmState(const AlgorithmStartupOptions &options)
{
    StateNode node = initialStateNode(options.generationOptions, init.getRng());
    globalState.initialStateNode = std::make_unique<StateNode>(std::move(node));
    globalState.algorithm = getAlgorithm(options.algorithmName);
    globalState.iterationCount = options.iterations;
}

// Binding: Get pointer to current initialStateNode in global state
StateNode *getInitialStateNode()
{
    return globalState.initialStateNode.get();
}

EMSCRIPTEN_BINDINGS(RandomGraphOptions)
{
    value_object<RandomGraphOptions>("RandomGraphOptions")
        .field("numVertices", &RandomGraphOptions::numVertices)
        .field("numEdges", &RandomGraphOptions::numEdges)
        .field("allowSelfLoops", &RandomGraphOptions::allowSelfLoops);
}

EMSCRIPTEN_BINDINGS(AlgorithmStartupOptions)
{
    value_object<AlgorithmStartupOptions>("AlgorithmStartupOptions")
        .field("algorithmName", &AlgorithmStartupOptions::algorithmName)
        .field("iterations", &AlgorithmStartupOptions::iterations)
        .field("generationOptions", &AlgorithmStartupOptions::generationOptions);
}

EMSCRIPTEN_BINDINGS(Color)
{
    value_object<Color>("Color")
        .field("index", &Color::index)
        .field("r", &Color::r)
        .field("g", &Color::g)
        .field("b", &Color::b);
}

EMSCRIPTEN_BINDINGS(GraphNode)
{
    register_vector<std::shared_ptr<GraphNode>>("VectorGraphNodePtr");

    class_<GraphNode>("GraphNode")
        .smart_ptr<std::shared_ptr<GraphNode>>("GraphNode")
        .property("neighbors", &GraphNode::getNeighbors);
}

EMSCRIPTEN_BINDINGS(ColorPalette)
{
    register_vector<Color>("ColorPaletteVector");

    class_<ColorPalette>("ColorPalette")
        .constructor<int>()
        .property("colors", &ColorPalette::getColors);
}

EMSCRIPTEN_BINDINGS(StateNode)
{
    // Register map and vector types for JS interop
    register_map<std::shared_ptr<GraphNode>, Color>("ColoringMap");
    register_map<int, int>("UsedColorsMap");
    register_vector<int>("VectorInt");
    class_<StateNode>("StateNode")
        .property("graph", &StateNode::graph)
        .property("palette", &StateNode::palette)
        .property("coloring", &StateNode::coloring)
        .property("conflicts", &StateNode::conflicts)
        .property("lastUsedColorIndex", &StateNode::lastUsedColorIndex)
        .property("usedColors", &StateNode::usedColors)
        .function("computeH", &StateNode::computeH);
}

EMSCRIPTEN_BINDINGS(Graph)
{
    class_<Graph>("Graph")
        .smart_ptr<std::shared_ptr<Graph>>("Graph")
        .function("getNodes", &Graph::getNodes);
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("setInitialAlgorithmState", &setInitialAlgorithmState);
    function("getInitialStateNode", &getInitialStateNode, allow_raw_pointers());
}
