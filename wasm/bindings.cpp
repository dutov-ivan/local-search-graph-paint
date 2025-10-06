
#include <emscripten/bind.h>
#include "graph.h"
#include "algorithms.h"
#include <memory>
#include <optional>
#include "init.h"
#include <unordered_map>
#include <emscripten/val.h>
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
    return StateNode{graph, std::move(palette), std::move(coloring), conflicts, std::move(usedColors)};
}

std::unique_ptr<AlgorithmIterator> initializeAlgorithm(std::unique_ptr<StateNode> initialState, const std::string &algorithmName, int iterations)
{
    if (algorithmName == "hill_climbing")
    {
        return std::make_unique<HillClimbingColoringIterator>(std::move(initialState), iterations, init.getRng());
    }
    else if (algorithmName == "simulated_annealing")
    {
        return std::make_unique<SimulatedAnnealingColoringIterator>(std::move(initialState), iterations, init.getRng());
    }
    else if (algorithmName == "beam")
    {
        return std::make_unique<BeamColoringIterator>(std::move(initialState), iterations, init.getRng());
    }
    else
    {
        throw std::invalid_argument("Unknown algorithm name: " + algorithmName);
    }
}

// Binding: Generate and set initialStateNode in global state, return it

void setInitialAlgorithmState(const AlgorithmStartupOptions &options)
{
    // Create fresh initial state
    StateNode node = initialStateNode(options.generationOptions, init.getRng());
    // Store a preserved copy for retrieval (shared_ptr graph so shallow share is fine)
    globalState.initialStateNode = std::make_unique<StateNode>(node.graph, node.palette, node.coloring, node.conflicts, node.usedColors);

    // Create a separate working copy for the algorithm iterator so that the preserved
    // initial state remains accessible to JS unchanged.
    auto workingCopy = std::make_unique<StateNode>(node.graph, node.palette, node.coloring, node.conflicts, node.usedColors);

    globalState.algorithm = initializeAlgorithm(std::move(workingCopy), options.algorithmName, options.iterations);
    globalState.iterationCount = options.iterations; // store requested iteration limit
}

// Binding: Get pointer to current initialStateNode in global state
std::shared_ptr<StateNode> getInitialStateNode()
{
    return globalState.initialStateNode;
}

// ---------- Helper data extraction (value-oriented) ----------

// Build adjacency as JS array-of-arrays (each inner array holds neighbor indices)
emscripten::val getGraphAdjacency(std::shared_ptr<Graph> graph)
{
    using emscripten::val;
    val adj = val::array();
    if (!graph)
        return adj;
    const auto &nodes = graph->getNodes();
    std::unordered_map<GraphNode *, int> index;
    index.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        index[nodes[i].get()] = static_cast<int>(i);
    }
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        val nbrs = val::array();
        int pos = 0;
        for (auto &nbr : nodes[i]->getNeighbors())
        {
            auto it = index.find(nbr.get());
            if (it != index.end())
            {
                nbrs.set(pos++, it->second);
            }
        }
        adj.set(i, nbrs);
    }
    return adj;
}

// Internal helper to convert a StateNode's coloring into an array (by node index) of color objects
emscripten::val stateColorArray(const StateNode &state)
{
    using emscripten::val;
    val arr = val::array();
    if (!state.graph)
        return arr;
    const auto &nodes = state.graph->getNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        auto it = state.coloring.find(nodes[i]);
        if (it != state.coloring.end())
        {
            const Color &c = it->second;
            val obj = val::object();
            obj.set("index", c.index);
            obj.set("r", c.r);
            obj.set("g", c.g);
            obj.set("b", c.b);
            arr.set(i, obj);
        }
        else
        {
            arr.set(i, val::undefined());
        }
    }
    return arr;
}

emscripten::val getInitialColorArray()
{
    if (!globalState.initialStateNode)
        return emscripten::val::array();
    return stateColorArray(*globalState.initialStateNode);
}

emscripten::val getCurrentColorArray()
{
    if (globalState.algorithm)
    {
        return stateColorArray(globalState.algorithm->getState());
    }
    return getInitialColorArray();
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
        // Revert to default copy semantics to avoid binding issues with const refs.
        // Frontend now avoids this path (uses adjacency helpers), so occasional copies are acceptable.
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
        .smart_ptr<std::shared_ptr<StateNode>>("StateNode")
        .property("graph", &StateNode::graph)
        .property("palette", &StateNode::palette)
        .property("coloring", &StateNode::coloring)
        .property("node", &StateNode::node)
        .property("color", &StateNode::color)
        .property("conflicts", &StateNode::conflicts)
        .property("continueIteration", &StateNode::continueIteration)
        .property("usedColors", &StateNode::usedColors)
        .function("computeH", &StateNode::computeH);
}

EMSCRIPTEN_BINDINGS(Graph)
{
    class_<Graph>("Graph")
        .smart_ptr<std::shared_ptr<Graph>>("Graph")
        // Using default behavior (copy). Visualization now uses getGraphAdjacency() so this is rarely invoked.
        .function("getNodes", &Graph::getNodes);
}

// Run greedy removal on the current algorithm state and recompute conflicts/usage
void runGreedyRemoveConflicts()
{
    if (!globalState.algorithm)
        throw std::runtime_error("Algorithm not initialized");
    StateNode &st = const_cast<StateNode &>(globalState.algorithm->getState());
    greedyRemoveConflicts(st);
    st.conflicts = computeConflicts(*st.graph, st.coloring);
    // recompute usedColors map
    st.usedColors.clear();
    for (auto &kv : st.coloring)
    {
        st.usedColors[kv.second.index]++;
    }
}

// Reinitialize algorithm iterator from preserved initialStateNode without regenerating graph
void reinitializeAlgorithm(const std::string &algorithmName, int iterations)
{
    if (!globalState.initialStateNode)
        throw std::runtime_error("No preserved initial state to reinitialize from");
    // Make a working copy so original stays immutable for further resets
    auto workingCopy = std::make_unique<StateNode>(*globalState.initialStateNode);
    globalState.algorithm = initializeAlgorithm(std::move(workingCopy), algorithmName, iterations);
    globalState.iterationCount = iterations;
}

EMSCRIPTEN_BINDINGS(StepResult)
{
    value_object<StepResult>("StepResult")
        .field("node", &StepResult::node)
        .field("color", &StepResult::color)
        .field("conflicts", &StepResult::conflicts)
        .field("continueIteration", &StepResult::continueIteration);
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("setInitialAlgorithmState", &setInitialAlgorithmState);
    function("getInitialStateNode", &getInitialStateNode);
    // Value extraction helpers (arrays only; no GraphNode wrappers required for visualization)
    function("getGraphAdjacency", &getGraphAdjacency);
    function("getInitialColorArray", &getInitialColorArray);
    function("getCurrentColorArray", &getCurrentColorArray);
    // New algorithm control bindings
    // step now returns pair<int, Color>
    function("algorithmStep", +[]() -> StepResult
             {
        if (!globalState.algorithm)
            throw std::runtime_error("Algorithm not initialized");
        StateNode st = globalState.algorithm->step(); // copy
        return StepResult(st.node, st.color, st.conflicts, st.continueIteration); });
    function("algorithmRunToEnd", +[]()
                                  {
        if (!globalState.algorithm)
            throw std::runtime_error("Algorithm not initialized");
        globalState.algorithm->runToEnd(); });
    function("getCurrentAlgorithmState", +[]() -> StateNode *
             {
        if (!globalState.algorithm)
            return nullptr;
        // getState returns const ref; cast away const for embind (JS won't mutate internal fields directly)
        return const_cast<StateNode *>(&globalState.algorithm->getState()); }, allow_raw_pointers());
    function("runGreedyRemoveConflicts", &runGreedyRemoveConflicts);
    // Reset only the active algorithm iterator, preserving the stored initialStateNode so JS can re-use it.
    function("resetAlgorithm", +[]()
                               {
        globalState.algorithm.reset();
        globalState.iterationCount = 0; });
    function("reinitializeAlgorithm", &reinitializeAlgorithm);
    function("getCurrentIteration", +[]() -> int
             {
        if (!globalState.algorithm) return 0;
        return globalState.algorithm->currentIteration(); });
}
