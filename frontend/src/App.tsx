import { useState, useEffect } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Play, FastForward } from 'lucide-react'
import './App.css'
import AdjacencyGraphViewer from './components/graph/AdjacencyGraphViewer'
import factory, { type MainModule, type StateNode, type AlgorithmStartupOptions, type Graph, type ColorPalette, type ColoringMap } from "../../build/GraphColoring.js"

type AlgorithmState = {
  graph: Graph, // retained for algorithms, but not needed for visualization anymore
  conflicts: number,
  lastUsedColor: number,
  palette: ColorPalette,
  coloringMap: ColoringMap
  adjacency: number[][],
  colorArray: { index: number; r: number; g: number; b: number; }[]
}

function App() {
  const [iterations, setIterations] = useState('10000')
  const [vertices, setVertices] = useState(50)
  const [edges, setEdges] = useState(120)
  // Holds the initial algorithm state (nodes, conflicts, lastUsedColor, paletteSize, etc)
  const [algorithmState, setAlgorithmState] = useState<AlgorithmState | null>(null);
  const [wasmModule, setWasmModule] = useState<MainModule | null>(null)
  const [algorithmName, setAlgorithmName] = useState<string>('hill_climbing');

  // Load WASM module once
  useEffect(() => {
    factory().then((Module) => setWasmModule(Module))
  }, [])

  const deletePreviousAlgorithmState = () =>{
    if (algorithmState) {
      // We no longer iterate node wrappers, just delete the graph & palette/coloring maps if desired.
      try { algorithmState.graph.delete(); } catch { /* ignore */ }
    }
  }

  const generateInitialState = () => {
    if (!wasmModule) return
    // Set initial algorithm state in WASM
    const startupOptions: AlgorithmStartupOptions = {
      algorithmName,
      iterations: Number(iterations),
      generationOptions: {
        numVertices: vertices,
        numEdges: edges,
        allowSelfLoops: false,
      }
    }
    wasmModule.setInitialAlgorithmState(startupOptions)
    const stateNode: StateNode | null = wasmModule.getInitialStateNode()
    if (!stateNode) return
    // Extract graph nodes
    const graph = stateNode.graph
    console.log("Generated graph:", graph);
    // Build adjacency & color arrays from C++ helpers
    const adjacencyVal = wasmModule.getGraphAdjacency(graph);
    const adjacency: number[][] = [];
    for (let i = 0; i < adjacencyVal.length; i++) {
      const row = adjacencyVal[i];
      const inner: number[] = [];
      for (let j = 0; j < row.length; j++) inner.push(row[j]);
      adjacency.push(inner);
    }
    const colorArrayVal = wasmModule.getInitialColorArray();
    const colorArray: { index: number; r: number; g: number; b: number; }[] = [];
    for (let i = 0; i < colorArrayVal.length; i++) {
      const c = colorArrayVal[i];
      if (c) colorArray.push({ index: c.index, r: c.r, g: c.g, b: c.b }); else colorArray.push({ index: -1, r: 0, g:0, b:0 });
    }
    deletePreviousAlgorithmState();
    setAlgorithmState({
      graph: graph!,
      conflicts: stateNode.conflicts,
      lastUsedColor: stateNode.lastUsedColorIndex,
      palette: stateNode.palette!,
      coloringMap: stateNode.coloring!,
      adjacency,
      colorArray,
    })
  }

  const stepAlgorithm = () => {
    if (!wasmModule) {
      console.log("WASM module not loaded yet.");
      return;
    }

    if (!algorithmState){
      console.log("No algorithm state available to step.");
      return;
    }
    try {
      wasmModule.algorithmStep();
      const stateNode: StateNode | null = wasmModule.getCurrentAlgorithmState();
      console.log("Stepped algorithm, new state:", stateNode);
      if (stateNode) {
        const graph = stateNode.graph!;
        const adjacencyVal = wasmModule.getGraphAdjacency(graph);
        const adjacency: number[][] = [];
        for (let i = 0; i < adjacencyVal.length; i++) {
          const row = adjacencyVal[i];
          const inner: number[] = [];
          for (let j = 0; j < row.length; j++) inner.push(row[j]);
          adjacency.push(inner);
        }
        const colorArrayVal = wasmModule.getCurrentColorArray();
        const colorArray: { index: number; r: number; g: number; b: number; }[] = [];
        for (let i = 0; i < colorArrayVal.length; i++) {
          const c = colorArrayVal[i];
          if (c) colorArray.push({ index: c.index, r: c.r, g: c.g, b: c.b }); else colorArray.push({ index: -1, r: 0, g:0, b:0 });
        }
        setAlgorithmState((prev) => prev ? ({
          graph,
          conflicts: stateNode.conflicts,
          coloringMap: stateNode.coloring!,
          lastUsedColor: stateNode.lastUsedColorIndex,
          palette: stateNode.palette!,
          adjacency,
          colorArray,
        }) : prev);
      }
      // Optionally could mark finished when cont is false
    } catch (e) {
      console.error(e);
    }
  }

  const runAlgorithmToEnd = () => {
    if (!wasmModule) return;
    if (!algorithmState) return;
    try {
      wasmModule.algorithmRunToEnd();
      const stateNode: StateNode | null = wasmModule.getCurrentAlgorithmState();
      if (stateNode) {
        const graph = stateNode.graph!;
        const adjacencyVal = wasmModule.getGraphAdjacency(graph);
        const adjacency: number[][] = [];
        for (let i = 0; i < adjacencyVal.length; i++) {
          const row = adjacencyVal[i];
          const inner: number[] = [];
          for (let j = 0; j < row.length; j++) inner.push(row[j]);
          adjacency.push(inner);
        }
        const colorArrayVal = wasmModule.getCurrentColorArray();
        const colorArray: { index: number; r: number; g: number; b: number; }[] = [];
        for (let i = 0; i < colorArrayVal.length; i++) {
          const c = colorArrayVal[i];
          if (c) colorArray.push({ index: c.index, r: c.r, g: c.g, b: c.b }); else colorArray.push({ index: -1, r: 0, g:0, b:0 });
        }
        setAlgorithmState((prev) => prev ? ({
          graph,
          conflicts: stateNode.conflicts,
          lastUsedColor: stateNode.lastUsedColorIndex,
          palette: stateNode.palette!,
          coloringMap: stateNode.coloring!,
          adjacency,
          colorArray,
        }) : prev);
      }
    } catch (e) {
      console.error(e);
    }
  }

  return (
    <div className="h-screen bg-white overflow-hidden lg:overflow-hidden overflow-y-auto">
      <div className="lg:h-full min-h-screen lg:min-h-0 p-4 flex flex-col">
        <div className="flex flex-col gap-4 lg:gap-4 lg:flex-row flex-1 min-h-0 max-w-7xl mx-auto w-full">
          {/* Graph Visualization Section */}
          <div className="flex-1 flex flex-col min-h-0">
            {/* Graph Data Header */}
            <div className="mb-2 flex flex-wrap items-center gap-3 flex-shrink-0">
              <div className="text-sm">
                <span className="text-gray-600">Current iterations: </span>
                <span className="font-medium">{iterations}</span>
              </div>
              <div className="text-sm">
                <span className="text-gray-600">Conflicts: </span>
                <span className="font-medium">{algorithmState?.conflicts ?? "0"}</span>
              </div>
              <div className="flex gap-2">
                <div className="rounded bg-gray-200 px-3 py-1 text-xs">
                  {algorithmState?.lastUsedColor ?? 'None'}
                </div>
                <div className="rounded bg-gray-200 px-3 py-1 text-xs">
                  {algorithmState?.palette?.colors.size() ?? '0'}
                </div>
              </div>
            </div>

            {/* Graph Area */}
            <Card className="mb-2 flex-1 lg:flex-1 bg-gray-200 flex">
              <CardContent className="flex h-[60vh] w-full p-0 flex-1">
                {algorithmState?.graph ? (
                  <AdjacencyGraphViewer adjacency={algorithmState.adjacency} colors={algorithmState.colorArray} />
                ) : (
                  <div className="m-auto text-gray-500 text-sm select-none">Generate a graph to visualize</div>
                )}
              </CardContent>
            </Card>

            {/* Control Buttons */}
            <div className="flex gap-2 flex-shrink-0">
              <Button size="sm" variant="outline" className="h-8 w-8 p-0" onClick={stepAlgorithm}>
                <Play className="h-3 w-3"  />
              </Button>
              <Button size="sm" variant="outline" className="h-8 w-8 p-0" onClick={runAlgorithmToEnd}>
                <FastForward className="h-3 w-3"  />
              </Button>
              <Button size="sm" variant="outline" className="h-8 px-3">
                Reset
              </Button>
            </div>
          </div>

          {/* Control Panel */}
          <div className="lg:w-72 flex-shrink-0">
            <Card className="lg:h-full h-auto">
              <CardContent className="p-3 lg:h-full flex flex-col lg:overflow-hidden">
                {/* Graph Properties */}
                <div className="space-y-2 mb-3 flex-shrink-0">
                  <div>
                    <Label htmlFor="vertices" className="text-xs">
                      Vertices:
                    </Label>
                    <Input
                      id="vertices"
                      type='number'
                      value={vertices}
                      onChange={(e) => setVertices(+e.target.value)}
                      className="mt-1 h-8 text-xs"
                      placeholder="Number of vertices"
                    />
                  </div>
                  
                  <div>
                    <Label htmlFor="edges" className="text-xs">
                      Edges:
                    </Label>
                    <Input
                      id="edges"
                      type='number'
                      value={edges}
                      onChange={(e) => setEdges(+e.target.value)}
                      className="mt-1 h-8 text-xs"
                      placeholder="Number of edges"
                    />
                  </div>
                </div>

                {/* Algorithm Settings */}
                <div className="space-y-2 mb-3 flex-shrink-0">
                  <div>
                    <Label htmlFor="algorithm" className="text-xs">
                      Algorithm dropdown
                    </Label>
                      <Select value={algorithmName} onValueChange={setAlgorithmName}>
                        <SelectTrigger className="mt-1 h-8 text-xs">
                          <SelectValue placeholder="Select algorithm" />
                        </SelectTrigger>
                        <SelectContent>
                          <SelectItem value="hill_climbing">Hill Climbing</SelectItem>
                          <SelectItem value="simulated_annealing">Simulated Annealing</SelectItem>
                          <SelectItem value="beam">Beam Search</SelectItem>
                        </SelectContent>
                      </Select>
                  </div>

                  <div>
                    <Label htmlFor="iterations" className="text-xs">
                      Iterations
                    </Label>
                    <Input
                      id="iterations"
                      value={iterations}
                      onChange={(e) => setIterations(e.target.value)}
                      className="mt-1 h-8 text-xs"
                      placeholder="Number of iterations"
                    />
                  </div>
                </div>

                {/* Generate Button */}
                <div className="mt-auto flex-shrink-0">
                  <Button className="w-full bg-red-300 text-black hover:bg-red-400" onClick={generateInitialState}>
                    Generate initial state
                  </Button>
                </div>
              </CardContent>
            </Card>
          </div>
        </div>
      </div>
    </div>
  )
}

export default App
