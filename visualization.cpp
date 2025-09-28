// Visualization utilities for Graph and ColoringMap
#include "visualization.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>

static std::string colorToHex(const Color &c)
{
    auto toHex = [](int v)
    {
        if (v < 0)
            v = 0;
        if (v > 255)
            v = 255;
        std::ostringstream oss;
        oss.setf(std::ios::hex, std::ios::basefield);
        oss.width(2);
        oss.fill('0');
        oss << std::uppercase << v;
        std::string s = oss.str();
        if (s.size() == 1)
            s = "0" + s;
        return s;
    };
    return "#" + toHex(c.r) + toHex(c.g) + toHex(c.b);
}

bool writeGraphToDot(const Graph &graph,
                     const ColoringMap *coloring,
                     const std::string &dotFilePath,
                     bool labelWithColorOrder)
{
    std::ofstream ofs(dotFilePath);
    if (!ofs.is_open())
        return false;

    // Map Node* to an index for stable labels
    std::unordered_map<const Node *, int> indexOf;
    int idx = 0;
    for (const auto node : graph.getNodes())
    {
        indexOf[node] = idx++;
    }

    ofs << "graph G {\n";
    ofs << "  node [shape=circle, style=filled, fontsize=12];\n";
    ofs << "  overlap=false;\n";

    // Emit nodes
    for (const auto node : graph.getNodes())
    {
        const int id = indexOf[node];
        std::string label = std::to_string(id);
        std::string fillColor = "#FFFFFF"; // default white
        std::string fontColor = "#000000";
        if (coloring)
        {
            auto it = coloring->find(const_cast<Node *>(node));
            if (it != coloring->end())
            {
                const Color &c = it->second;
                fillColor = colorToHex(c);
                if (labelWithColorOrder)
                    label = std::to_string(c.order);
            }
        }
        ofs << "  n" << id << " [label=\"" << label << "\", fillcolor=\"" << fillColor << "\", fontcolor=\"" << fontColor << "\"];\n";
    }

    // Emit edges (allowing parallel edges)
    // We'll output an undirected edge for each neighbor entry where u's index <= v's index
    // to avoid duplicating the same undirected edge twice in the DOT (but allow parallel edges by not de-duplicating pairs occurring multiple times).
    for (const auto u : graph.getNodes())
    {
        const int uid = indexOf[u];
        for (const auto v : u->getNeighbors())
        {
            const int vid = indexOf[v];
            if (uid <= vid)
            {
                ofs << "  n" << uid << " -- n" << vid << ";\n";
            }
        }
    }

    ofs << "}\n";
    return true;
}

bool renderDotToImage(const std::string &dotFilePath,
                      const std::string &outputImagePath,
                      const std::string &engine,
                      const std::string &format)
{
    // Build command: <engine> -T<format> <dotFilePath> -o <outputImagePath>
    std::ostringstream cmd;
    // On Windows, avoid extra quotes around executable and file paths
    cmd << engine << " -T" << format << " " << dotFilePath << " -o " << outputImagePath;
    int code = std::system(cmd.str().c_str());
    return code == 0;
}

bool visualizeGraph(const Graph &graph,
                    const ColoringMap *coloring,
                    const std::string &dotFilePath,
                    const std::string &outputImagePath,
                    const std::string &engine,
                    const std::string &format,
                    bool labelWithColorOrder)
{
    if (!writeGraphToDot(graph, coloring, dotFilePath, labelWithColorOrder))
        return false;
    return renderDotToImage(dotFilePath, outputImagePath, engine, format);
}
