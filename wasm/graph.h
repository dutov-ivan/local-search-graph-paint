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
    void addNeighbor(const std::shared_ptr<GraphNode> &neighbor);
    const std::vector<std::shared_ptr<GraphNode>> &getNeighbors() const;

private:
    std::vector<std::shared_ptr<GraphNode>> neighbors;
};

class Graph
{
public:
    void addNode(const std::shared_ptr<GraphNode> &node);
    void addEdge(const std::shared_ptr<GraphNode> &a, const std::shared_ptr<GraphNode> &b);
    void reserveNodes(std::size_t n);
    const std::vector<std::shared_ptr<GraphNode>> &getNodes() const;

private:
    std::vector<std::shared_ptr<GraphNode>> nodes_;
};

struct RandomGraphOptions
{
    std::size_t numVertices;
    std::size_t numEdges;
    bool allowSelfLoops = false;
};

Graph generateRandomGraph(const RandomGraphOptions &options, std::mt19937 &rng);
#endif