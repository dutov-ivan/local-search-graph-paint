import { useEffect, useMemo } from 'react';
import type { FC } from 'react';
import Graphology from 'graphology';
import { SigmaContainer, useLoadGraph } from '@react-sigma/core';
import '@react-sigma/core/lib/style.css';

export interface AdjacencyGraphViewerProps {
  adjacency: number[][]; // array of neighbor index arrays
  colors?: { index: number; r: number; g: number; b: number; }[] | (undefined | { index: number; r: number; g: number; b: number; })[];
  className?: string;
}

function buildGraph(adjacency: number[][], colors?: (undefined | { index: number; r: number; g: number; b: number; })[]) {
  const g = new Graphology();
  const n = adjacency.length;
  const radius = Math.max(50, n * 2);
  for (let i = 0; i < n; i++) {
    const angle = (i / Math.max(1, n)) * Math.PI * 2;
    const x = Math.cos(angle) * radius;
    const y = Math.sin(angle) * radius;
    let color: string | undefined;
    const c = colors?.[i];
    if (c) {
      const toHex = (v: number) => v.toString(16).padStart(2, '0');
      color = `#${toHex(c.r)}${toHex(c.g)}${toHex(c.b)}`;
    }
    g.addNode(`n${i}`, { x, y, size: 5, label: `n${i}`, color });
  }
  const seen = new Set<string>();
  for (let i = 0; i < n; i++) {
    for (const j of adjacency[i]) {
      if (j === i) continue;
      const a = Math.min(i, j);
      const b = Math.max(i, j);
      const key = `${a}-${b}`;
      if (!seen.has(key)) {
        seen.add(key);
        try { g.addEdge(`n${a}`, `n${b}`, { size: 2, color: '#000' }); } catch { /* ignore */ }
      }
    }
  }
  return g;
}

const Loader: FC<AdjacencyGraphViewerProps> = ({ adjacency, colors }) => {
  const loadGraph = useLoadGraph();
  const instance = useMemo(() => buildGraph(adjacency, colors as any), [adjacency, colors]);
  useEffect(() => { loadGraph(instance); }, [instance, loadGraph]);
  return null;
};

export const AdjacencyGraphViewer: FC<AdjacencyGraphViewerProps> = ({ adjacency, colors, className }) => (
  <SigmaContainer style={{ width: '100%', height: '100%', minHeight: '400px' }} className={className}>
    <Loader adjacency={adjacency} colors={colors} />
  </SigmaContainer>
);

export default AdjacencyGraphViewer;
