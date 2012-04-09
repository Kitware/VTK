package require vtk
package require vtkinteraction

set file0 rgFile0.vtr
set file1 rgFile1.vtr
set file2 rgFile2.vtr

# read in some grid data
vtkRectilinearGridReader gridReader
  gridReader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
  gridReader Update

# extract to reduce extents of grid
vtkExtractRectilinearGrid extract
  extract SetInputConnection [gridReader GetOutputPort]
  extract SetVOI 0 23 0 32 0 10
  extract Update

# write just a piece (extracted piece) as well as the whole thing
vtkXMLRectilinearGridWriter rgWriter
  rgWriter SetFileName $file0
  rgWriter SetInputConnection [extract GetOutputPort]
  rgWriter SetDataModeToAscii
  rgWriter Write

  rgWriter SetFileName $file1
  rgWriter SetInputConnection [gridReader GetOutputPort]
  rgWriter SetDataModeToAppended
  rgWriter SetNumberOfPieces 2
  rgWriter Write

  rgWriter SetFileName $file2
  rgWriter SetDataModeToBinary
  rgWriter SetWriteExtent 3 46 6 32 1 5
  rgWriter SetCompressor ""
  if {[rgWriter GetByteOrder]} {
    rgWriter SetByteOrder 0
  } else {
    rgWriter SetByteOrder 1
  }
  rgWriter Write

# read the extracted grid
vtkXMLRectilinearGridReader reader
  reader SetFileName $file0
  reader WholeSlicesOff
  reader Update

vtkRectilinearGrid rg0
  rg0 DeepCopy [reader GetOutput]

vtkDataSetMapper mapper0
  mapper0 SetInputData rg0

vtkActor actor0
  actor0 SetMapper mapper0

# read the whole grid
reader SetFileName $file1
reader WholeSlicesOn
reader Update

vtkRectilinearGrid rg1
  rg1 DeepCopy [reader GetOutput]

vtkDataSetMapper mapper1
  mapper1 SetInputData rg1

vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetPosition -1.5 3 0

# read the partially written grid
reader SetFileName $file2
reader Update

vtkDataSetMapper mapper2
  mapper2 SetInputConnection [reader GetOutputPort]

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 1.5 3 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor0
ren1 AddActor actor1
ren1 AddActor actor2

renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

file delete -force $file0
file delete -force $file1
file delete -force $file2
