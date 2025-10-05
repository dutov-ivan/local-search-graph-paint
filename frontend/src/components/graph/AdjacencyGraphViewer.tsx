import { useEffect, useMemo, useState } from 'react';
import type { FC } from 'react';
import Graphology from 'graphology';
import { SigmaContainer, useLoadGraph, useSigma, useRegisterEvents, useSetSettings, useCamera } from '@react-sigma/core';
import '@react-sigma/core/lib/style.css';

export interface AdjacencyGraphViewerProps {
  adjacency: number[][]; // array of neighbor index arrays
  colors?: { index: number; r: number; g: number; b: number; }[] | (undefined | { index: number; r: number; g: number; b: number; })[];
  className?: string;
  disableHoverEffect?: boolean;
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

const HoverEffects: FC<{ disableHoverEffect?: boolean }> = ({ disableHoverEffect }) => {
  const sigma = useSigma();
  const registerEvents = useRegisterEvents();
  const setSettings = useSetSettings();
  const [hoveredNode, setHoveredNode] = useState<string | null>(null);

  /**
   * Register hover events
   */
  useEffect(() => {
    registerEvents({
      enterNode: (event) => setHoveredNode(event.node),
      leaveNode: () => setHoveredNode(null),
    });
  }, [registerEvents]);

  /**
   * When hovered node changes, update the node and edge reducers
   */
  useEffect(() => {
    setSettings({
      nodeReducer: (node, data) => {
        const graph = sigma.getGraph();
        const newData = { ...data, highlighted: data.highlighted || false };

        if (!disableHoverEffect && hoveredNode) {
          if (node === hoveredNode || graph.neighbors(hoveredNode).includes(node)) {
            newData.highlighted = true;
          } else {
            (newData as any).color = '#E2E2E2';
            newData.highlighted = false;
          }
        }
        return newData;
      },
      edgeReducer: (edge, data) => {
        const graph = sigma.getGraph();
        const newData = { ...data, hidden: false };

        if (!disableHoverEffect && hoveredNode && !graph.extremities(edge).includes(hoveredNode)) {
          newData.hidden = true;
        }
        return newData;
      },
    });
  }, [hoveredNode, setSettings, sigma, disableHoverEffect]);

  return null;
};


export const FocusOnNode: FC<{ node: string | null; move?: boolean }> = ({ node, move }) => {
  // Get sigma
  const sigma = useSigma();
  // Get camera hook
  const { gotoNode } = useCamera();

  /**
   * When the selected item changes, highlighted the node and center the camera on it.
   */
  useEffect(() => {
    if (!node) return;
    sigma.getGraph().setNodeAttribute(node, 'highlighted', true);
    if (move) gotoNode(node);

    return () => {
      sigma.getGraph().setNodeAttribute(node, 'highlighted', false);
    };
  }, [node, move, sigma, gotoNode]);

  return null;
};


export const AdjacencyGraphViewer: FC<AdjacencyGraphViewerProps> = ({ adjacency, colors, className, disableHoverEffect }) => {
  const [selectedNode] = useState<string | null>(null);

  return (
  <SigmaContainer style={{ width: '100%', height: '100%', minHeight: '400px' }} className={className} settings={{ enableEdgeEvents: true }}>
    <Loader adjacency={adjacency} colors={colors} />
    <HoverEffects disableHoverEffect={disableHoverEffect} />
    <FocusOnNode node={selectedNode} />
  </SigmaContainer>
  )
};

export default AdjacencyGraphViewer;
