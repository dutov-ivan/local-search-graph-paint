#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "graph.h"
#include "algorithms.h"
#include <string>

// Write the graph (and optional coloring) to a Graphviz DOT file.
// - If coloring contains an entry for a node, the node will be filled with that color and
//   its label will be the color order; otherwise, the label will be the node index.
// - Nodes are rendered as circles.
// Returns true on success, false on failure.
bool writeGraphToDot(const Graph &graph,
                     const ColoringMap *coloring,
                     const std::string &dotFilePath,
                     bool labelWithColorOrder = true);

// Render a DOT file to an image using Graphviz.
// - engine: one of "dot", "neato", "sfdp", etc.
// - format: one of "png", "svg", "pdf", etc.
// Returns true on success (Graphviz exit code 0), false otherwise.
bool renderDotToImage(const std::string &dotFilePath,
                      const std::string &outputImagePath,
                      const std::string &engine = "dot",
                      const std::string &format = "png");

// Convenience helper: write a DOT file for the given graph and coloring and immediately render it.
// Returns true on success of both steps.
bool visualizeGraph(const Graph &graph,
                    const ColoringMap *coloring,
                    const std::string &dotFilePath,
                    const std::string &outputImagePath,
                    const std::string &engine = "dot",
                    const std::string &format = "png",
                    bool labelWithColorOrder = false);

#endif // VISUALIZATION_H
