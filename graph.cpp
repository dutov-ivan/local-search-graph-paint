#include "graph.h"
// Implementation details for graph generation
#include <random>
#include <chrono>
#include <unordered_map>

void Node::addNeighbor(Node *neighbor)
{
    neighbors.push_back(neighbor);
}

const std::vector<Node *> &Node::getNeighbors() const
{
    return neighbors;
}

void Graph::addNode(Node *node)
{
    nodes.push_back(node);
}

void Graph::addEdge(Node *a, Node *b)
{
    a->addNeighbor(b);
    b->addNeighbor(a);
}

const std::vector<Node *> &Graph::getNodes() const
{
    return nodes;
}

void Graph::generateRandomGraph(std::size_t numVertices, std::size_t numEdges, bool allowSelfLoops, unsigned int seed)
{
    // Clear any existing graph
    // Note: Nodes are raw pointers; ownership/lifetime management is out of scope here.
    // Caller is responsible for cleanup if needed. For simplicity, we just reset the container.
    nodes.clear();

    // Create vertices
    nodes.reserve(numVertices);
    for (std::size_t i = 0; i < numVertices; ++i)
    {
        nodes.push_back(new Node());
    }

    if (numVertices == 0)
        return;

    // Set up RNG
    unsigned int actualSeed = seed;
    if (actualSeed == 0)
    {
        actualSeed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    std::mt19937 rng(actualSeed);
    std::uniform_int_distribution<std::size_t> dist(0, numVertices - 1);

    // Track existing edges to prevent parallel edges
    std::unordered_map<std::size_t, std::size_t> existingEdges;
    std::size_t edgesAdded = 0;
    while (edgesAdded < numEdges)
    {
        std::size_t uIndex = dist(rng);
        std::size_t vIndex = dist(rng);

        if (!allowSelfLoops && numVertices > 1)
        {
            for (int attempts = 0; vIndex == uIndex && attempts < 5; ++attempts)
            {
                vIndex = dist(rng);
            }
        }

        // Always store edge as (min, max) for undirected graph
        std::size_t a = std::min(uIndex, vIndex);
        std::size_t b = std::max(uIndex, vIndex);
        if (!allowSelfLoops && a == b)
            continue;

        if (existingEdges[a] != b)
        {
            Node *u = nodes[uIndex];
            Node *v = nodes[vIndex];
            addEdge(u, v);
            existingEdges[a] = b;
            ++edgesAdded;
        }
        // else: skip, try again
    }
}