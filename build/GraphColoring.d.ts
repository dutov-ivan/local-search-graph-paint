// TypeScript bindings for emscripten-generated code.  Automatically generated at compile time.
interface WasmModule {
  _main(_0: number, _1: number): number;
}

type EmbindString = ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string;
export interface ClassHandle {
  isAliasOf(other: ClassHandle): boolean;
  delete(): void;
  deleteLater(): this;
  isDeleted(): boolean;
  // @ts-ignore - If targeting lower than ESNext, this symbol might not exist.
  [Symbol.dispose](): void;
  clone(): this;
}
export type RandomGraphOptions = {
  numVertices: number,
  numEdges: number,
  allowSelfLoops: boolean
};

export type AlgorithmStartupOptions = {
  algorithmName: EmbindString,
  iterations: number,
  generationOptions: RandomGraphOptions
};

export type Color = {
  index: number,
  r: number,
  g: number,
  b: number
};

export interface VectorGraphNodePtr extends ClassHandle {
  size(): number;
  get(_0: number): GraphNode | null | undefined;
  push_back(_0: GraphNode | null): void;
  resize(_0: number, _1: GraphNode | null): void;
  set(_0: number, _1: GraphNode | null): boolean;
}

export interface GraphNode extends ClassHandle {
  readonly neighbors: VectorGraphNodePtr;
}

export interface ColorPaletteVector extends ClassHandle {
  push_back(_0: Color): void;
  resize(_0: number, _1: Color): void;
  size(): number;
  get(_0: number): Color | undefined;
  set(_0: number, _1: Color): boolean;
}

export interface ColorPalette extends ClassHandle {
  readonly colors: ColorPaletteVector;
}

export interface ColoringMap extends ClassHandle {
  size(): number;
  get(_0: GraphNode | null): Color | undefined;
  set(_0: GraphNode | null, _1: Color): void;
  keys(): VectorGraphNodePtr;
}

export interface UsedColorsMap extends ClassHandle {
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): void;
  keys(): VectorInt;
}

export interface VectorInt extends ClassHandle {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): boolean;
}

export interface StateNode extends ClassHandle {
  palette: ColorPalette;
  coloring: ColoringMap;
  node: GraphNode | null;
  color: Color;
  conflicts: number;
  continueIteration: boolean;
  usedColors: UsedColorsMap;
  graph: Graph | null;
  computeH(): number;
}

export interface Graph extends ClassHandle {
  getNodes(): VectorGraphNodePtr;
}

export type StepResult = {
  node: GraphNode | null,
  color: Color,
  conflicts: number,
  continueIteration: boolean
};

interface EmbindModule {
  VectorGraphNodePtr: {
    new(): VectorGraphNodePtr;
  };
  GraphNode: {};
  ColorPaletteVector: {
    new(): ColorPaletteVector;
  };
  ColorPalette: {
    new(_0: number): ColorPalette;
  };
  ColoringMap: {
    new(): ColoringMap;
  };
  UsedColorsMap: {
    new(): UsedColorsMap;
  };
  VectorInt: {
    new(): VectorInt;
  };
  StateNode: {};
  Graph: {};
  setInitialAlgorithmState(_0: AlgorithmStartupOptions): void;
  getInitialStateNode(): StateNode | null;
  getGraphAdjacency(_0: Graph | null): any;
  getInitialColorArray(): any;
  getCurrentColorArray(): any;
  algorithmStep(): StepResult;
  algorithmRunToEnd(): void;
  getCurrentAlgorithmState(): StateNode | null;
  runGreedyRemoveConflicts(): void;
  resetAlgorithm(): void;
  reinitializeAlgorithm(_0: EmbindString, _1: number): void;
  getCurrentIteration(): number;
}

export type MainModule = WasmModule & EmbindModule;
export default function MainModuleFactory (options?: unknown): Promise<MainModule>;
