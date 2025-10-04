#include <emscripten/bind.h>
#include "graph.h"
#include "init.h"
using namespace emscripten;

Graph generateRandomGraphWithDefaultRNG(const RandomGraphOptions &options)
{
    return generateRandomGraph(options, init.getRng());
}

EMSCRIPTEN_BINDINGS(RandomGraphOptions)
{
    value_object<RandomGraphOptions>("RandomGraphOptions")
        .field("numVertices", &RandomGraphOptions::numVertices)
        .field("numEdges", &RandomGraphOptions::numEdges)
        .field("allowSelfLoops", &RandomGraphOptions::allowSelfLoops);
}

EMSCRIPTEN_BINDINGS(GraphNode)
{
    register_vector<GraphNode *>("VectorGraphNodePtr");
    class_<GraphNode>("GraphNode")
        .property("neighbors", &GraphNode::getNeighbors);
}

EMSCRIPTEN_BINDINGS(Graph)
{
    class_<Graph>("Graph")
        .function("getNodes", &Graph::getNodes);
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("generateRandomGraph", &generateRandomGraphWithDefaultRNG);
}
