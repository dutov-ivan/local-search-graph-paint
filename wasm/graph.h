#ifndef GRAPH_H
#define GRAPH_H

#include <vector>

class Node
{
public:
    Node() = default;
    void addNeighbor(Node *neighbor);
    const std::vector<Node *> &getNeighbors() const;

private:
    std::vector<Node *> neighbors;
};

class Graph
{
public:
    void addNode(Node *node);
    void addEdge(Node *a, Node *b);
    const std::vector<Node *> &getNodes() const;

    // Generate a graph with the specified number of vertices and edges.
    // - Parallel edges are allowed by design (multiple edges between the same pair of vertices).
    // - Self-loops are disabled by default but can be enabled via allowSelfLoops.
    // - If seed == 0, a non-deterministic seed will be used.
    void generateRandomGraph(std::size_t numVertices, std::size_t numEdges, bool allowSelfLoops = false, unsigned int seed = 0);

private:
    std::vector<Node *> nodes;
};

#endif