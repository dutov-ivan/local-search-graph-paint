#include "graph.h"
// Implementation details for graph generation
#include <random>
#include <chrono>
#include <unordered_map>

void GraphNode::addNeighbor(GraphNode *neighbor)
{
    neighbors.push_back(neighbor);
}

const std::vector<GraphNode *> &GraphNode::getNeighbors() const
{
    return neighbors;
}

void Graph::addNode(GraphNode *node)
{
    nodes_.push_back(node);
}

void Graph::addEdge(GraphNode *a, GraphNode *b)
{
    a->addNeighbor(b);
    b->addNeighbor(a);
}

void Graph::reserveNodes(std::size_t n)
{
    nodes_.reserve(n);
}

const std::vector<GraphNode *> &Graph::getNodes() const
{
    return nodes_;
}

Graph generateRandomGraph(const RandomGraphOptions &options, std::mt19937 &rng)

{
    Graph graph;
    if (options.numVertices == 0)
        return graph;

    // Create vertices
    graph.reserveNodes(options.numVertices);
    for (std::size_t i = 0; i < options.numVertices; ++i)
    {
        graph.addNode(new GraphNode());
    }

    // Set up RNG
    std::uniform_int_distribution<std::size_t> dist(0, options.numVertices - 1);

    // Track existing edges to prevent parallel edges
    std::unordered_map<std::size_t, std::size_t> existingEdges;
    std::size_t edgesAdded = 0;
    while (edgesAdded < options.numEdges)
    {
        std::size_t uIndex = dist(rng);
        std::size_t vIndex = dist(rng);

        if (!options.allowSelfLoops && options.numVertices > 1)
        {
            for (int attempts = 0; vIndex == uIndex && attempts < 5; ++attempts)
            {
                vIndex = dist(rng);
            }
        }

        // Always store edge as (min, max) for undirected graph
        std::size_t a = std::min(uIndex, vIndex);
        std::size_t b = std::max(uIndex, vIndex);
        if (!options.allowSelfLoops && a == b)
            continue;

        const std::vector<GraphNode *> &nodes = graph.getNodes();
        if (existingEdges[a] != b)
        {
            GraphNode *u = nodes[uIndex];
            GraphNode *v = nodes[vIndex];
            graph.addEdge(u, v);
            existingEdges[a] = b;
            ++edgesAdded;
        }
        // else: skip, try again
    }

    return graph;
}