#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <random>
#include <emscripten/bind.h>
#include "init.h"

class GraphNode
{
public:
    GraphNode() = default;
    void addNeighbor(GraphNode *neighbor);
    const std::vector<GraphNode *> &getNeighbors() const;

private:
    std::vector<GraphNode *> neighbors;
};

class Graph
{
public:
    void addNode(GraphNode *node);
    void addEdge(GraphNode *a, GraphNode *b);
    void reserveNodes(std::size_t n);
    const std::vector<GraphNode *> &getNodes() const;

    // Generate a graph with the specified number of vertices and edges.
    // - Parallel edges are allowed by design (multiple edges between the same pair of vertices).
    // - Self-loops are disabled by default but can be enabled via allowSelfLoops.
    // - If seed == 0, a non-deterministic seed will be used.

private:
    std::vector<GraphNode *> nodes_;
};

struct RandomGraphOptions
{
    std::size_t numVertices;
    std::size_t numEdges;
    bool allowSelfLoops = false;
};

Graph generateRandomGraph(const RandomGraphOptions &options, std::mt19937 &rng);
#endif