import { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Play, RotateCcw, FastForward } from 'lucide-react'
import './App.css'

function App() {
  const [iterations, setIterations] = useState('10000')
  const [vertices, setVertices] = useState('50')
  const [edges, setEdges] = useState('120')

  return (
    <div className="h-screen bg-white overflow-hidden lg:overflow-hidden overflow-y-auto">
      <div className="h-full lg:h-full min-h-screen lg:min-h-0 p-4 flex flex-col">
        <div className="flex flex-col gap-4 lg:gap-4 lg:flex-row flex-1 min-h-0 max-w-7xl mx-auto w-full">
          {/* Graph Visualization Section */}
          <div className="flex-1 flex flex-col min-h-0">
            {/* Graph Data Header */}
            <div className="mb-2 flex flex-wrap items-center gap-3 flex-shrink-0">
              <div className="text-sm">
                <span className="text-gray-600">Current iterations: </span>
                <span className="font-medium">100</span>
              </div>
              <div className="text-sm">
                <span className="text-gray-600">Conflicts: </span>
                <span className="font-medium">0</span>
              </div>
              <div className="flex gap-2">
                <div className="rounded bg-gray-200 px-3 py-1 text-xs">
                  LastUsedColor
                </div>
                <div className="rounded bg-gray-200 px-3 py-1 text-xs">
                  PaletteSize
                </div>
              </div>
            </div>

            {/* Graph Area */}
            <Card className="mb-2 flex-1 lg:flex-1 min-h-[50vh] bg-gray-200 flex">
              <CardContent className="flex h-full w-full items-center justify-center p-3 flex-1">
                <div className="text-gray-500">
                  {/* Graph visualization will go here */}
                  Graph Visualization Area
                </div>
              </CardContent>
            </Card>

            {/* Control Buttons */}
            <div className="flex gap-2 flex-shrink-0">
              <Button size="sm" variant="outline" className="h-8 w-8 p-0">
                <Play className="h-3 w-3" />
              </Button>
              <Button size="sm" variant="outline" className="h-8 w-8 p-0">
                <FastForward className="h-3 w-3" />
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
                      value={vertices}
                      onChange={(e) => setVertices(e.target.value)}
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
                      value={edges}
                      onChange={(e) => setEdges(e.target.value)}
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
                    <Select>
                      <SelectTrigger className="mt-1 h-8 text-xs">
                        <SelectValue placeholder="Select algorithm" />
                      </SelectTrigger>
                      <SelectContent>
                        <SelectItem value="greedy">Greedy</SelectItem>
                        <SelectItem value="dsatur">DSATUR</SelectItem>
                        <SelectItem value="welsh-powell">Welsh-Powell</SelectItem>
                        <SelectItem value="local-search">Local Search</SelectItem>
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
                  <Button className="w-full bg-red-300 text-black hover:bg-red-400">
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
