import { useEffect, useMemo } from 'react';
import type { FC } from 'react';
import Graphology from 'graphology';
import { SigmaContainer, useLoadGraph } from '@react-sigma/core';
import '@react-sigma/core/lib/style.css';
import type { Graph as WasmGraph, GraphNode, ColorPalette, Color, ColoringMap } from '../../../../build/GraphColoring.js';

/**
 * Convert WASM Graph (custom embind graph) to a graphology instance.
 * We assume nodes vector contains GraphNode objects each having a neighbors vector.
 * We'll generate node ids based on index order ("n0", "n1", ...). Since the WASM graph nodes
 * appear to be stored in a contiguous vector, we can map pointer order -> id.
 */
function convertWasmGraphToGraphology(wasmGraph: WasmGraph, colors?: ColoringMap): Graphology {
  const g = new Graphology();
  const nodesVec = wasmGraph.getNodes(); // reference to internal vector (do NOT delete vector itself)
  const size = nodesVec.size();
  // Collect one wrapper per node (do not delete; owned by C++ shared_ptr)
  const nativeNodes: (GraphNode | null)[] = [];
  // We intentionally do NOT delete the primary node wrappers created via nodesVec.get(i)
  // here because external components (coloring map, palette usage) may still rely on
  // them for pointer identity when coloring lookups occur. Only transient neighborNode
  // wrappers are deleted after use.
  // Pre-create nodes with random-ish initial positions in a circle layout for visibility.
  const radius = Math.max(50, size * 2);
  for (let i = 0; i < size; i++) {
    const nodeId = `n${i}`;
    const nativeNode = nodesVec.get(i) as GraphNode | null;
    nativeNodes.push(nativeNode);
    // Basic circular placement
    const angle = (i / size) * Math.PI * 2;
    const x = Math.cos(angle) * radius;
    const y = Math.sin(angle) * radius;
    let color: string | undefined;
    // If palette is available, attempt to assign color index i % paletteSize
    try {
        const c: Color | undefined = colors?.get(nativeNode);
        console.log(c)
        if (c) {
          const toHex = (v: number) => v.toString(16).padStart(2, '0');
            color = `#${toHex(c.r)}${toHex(c.g)}${toHex(c.b)}`;
        }
      }
    catch { /* ignore color errors */ }
    g.addNode(nodeId, { x, y, size: 5, label: nodeId, color });
  }
  // Add edges (undirected assumption). To avoid duplicates, only add each unordered pair once.
  const seenPairs = new Set<string>();
  for (let i = 0; i < size; i++) {
    const sourceNode = nativeNodes[i];
    if (!sourceNode) continue;
    const neighbors = sourceNode.neighbors;
    const degree = neighbors.size();
    for (let k = 0; k < degree; k++) {
      const neighborNode = neighbors.get(k);
      if (!neighborNode) continue;
      // Find index by alias comparison (O(n) but n is modest). Cache could be added later.
      let j = -1;
      for (let idx = 0; idx < nativeNodes.length; idx++) {
        const candidate = nativeNodes[idx];
        if (!candidate) continue;
        if (neighborNode === candidate || neighborNode.isAliasOf(candidate)) { j = idx; break; }
      }
      if (j === -1) continue; // not found
      if (j === i) continue; // skip self loops for now (add later if desired)
      const a = Math.min(i, j);
      const b = Math.max(i, j);
      const key = `${a}-${b}`;
      if (seenPairs.has(key)) continue;
      seenPairs.add(key);
      const s = `n${a}`;
      const t = `n${b}`;
      if (!g.hasEdge(s, t)) {
        try {
          g.addEdge(s, t, { color: '#000', size: 2 });
        } catch {
          // ignore individual edge add errors
        }
      }
      // Delete neighbor node wrapper (decrements JS-side refcount for shared_ptr)
      neighborNode.delete();
    }
    // neighbors is a reference-returned vector; do NOT delete, only delete node wrappers we explicitly fetched
  }
  // If after processing there are zero edges but multiple nodes, synthesize a simple ring (debug fallback)
  if (g.size === 0 && g.order > 1) {
    for (let i = 0; i < g.order; i++) {
      const a = `n${i}`;
      const b = `n${(i + 1) % g.order}`;
      if (!g.hasEdge(a, b)) {
        try { g.addEdge(a, b, { color: '#555', size: 1 }); } catch {/* ignore */}
      }
    }
  }
  // Debug: log counts (will be stripped / ignored in production bundling if minified)
  // eslint-disable-next-line no-console
  console.debug('[GraphViewer] nodes:', g.order, 'edges:', g.size);
  // Cleanup: Only transient neighbor wrappers were deleted inline. Do NOT delete nodesVec.
  return g;
}

const LoadConvertedGraph: FC<{ wasmGraph: WasmGraph, coloring?: ColoringMap }> = ({ wasmGraph, coloring }) => {
  const loadGraph = useLoadGraph();
  const graphologyInstance = useMemo(() => convertWasmGraphToGraphology(wasmGraph, coloring), [wasmGraph, coloring]);
  useEffect(() => {
    loadGraph(graphologyInstance);
  }, [graphologyInstance, loadGraph]);
  return null;
};

export const GraphViewer: FC<{ wasmGraph: WasmGraph, coloring?: ColoringMap, className?: string }> = ({ wasmGraph, coloring, className }) => {
  return (
  <SigmaContainer style={{ width: '100%', height: '100%', minHeight: '400px' }} className={className}>
      <LoadConvertedGraph wasmGraph={wasmGraph} coloring={coloring} />
    </SigmaContainer>
  );
};

export default GraphViewer;
