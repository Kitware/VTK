package require vtk
package require vtkinteraction

set file0 sgFile0.vts
set file1 sgFile1.vts
set file2 sgFile2.vts

# Create a reader and write out the field
vtkMultiBlockPLOT3DReader combReader
  combReader SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
  combReader SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
  combReader SetScalarFunctionNumber 100
  combReader Update
  set output [[combReader GetOutput] GetBlock 0]

# extract to reduce extents of grid
vtkExtractGrid extract
  extract SetInputData $output
  extract SetVOI 0 28 0 32 0 24
  extract Update

# write just a piece (extracted piece) as well as the whole thing
vtkXMLStructuredGridWriter gridWriter
  gridWriter SetFileName $file0
  gridWriter SetInputConnection [extract GetOutputPort]
  gridWriter SetDataModeToAscii
  gridWriter Write

  gridWriter SetInputData $output
  gridWriter SetFileName $file1
  gridWriter SetDataModeToAppended
  gridWriter SetNumberOfPieces 2
  gridWriter Write

  gridWriter SetFileName $file2
  gridWriter SetDataModeToBinary
  gridWriter SetWriteExtent 8 56 4 16 1 24
  gridWriter Write

# read the extracted grid
vtkXMLStructuredGridReader reader
  reader SetFileName $file0
  reader WholeSlicesOff
  reader Update

vtkStructuredGrid sg
  sg DeepCopy [reader GetOutput]

vtkContourFilter cF0
  cF0 SetInputData sg
  cF0 SetValue 0 0.38

vtkPolyDataMapper mapper0
  mapper0 SetInputConnection [cF0 GetOutputPort]
  mapper0 ScalarVisibilityOff

vtkActor actor0
  actor0 SetMapper mapper0


# read the whole image
reader SetFileName $file1
reader WholeSlicesOn
reader Update

vtkStructuredGrid sg1
  sg1 DeepCopy [reader GetOutput]

vtkContourFilter cF1
  cF1 SetInputData sg1
  cF1 SetValue 0 0.38

vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [cF1 GetOutputPort]
  mapper1 ScalarVisibilityOff

vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetPosition 0 -10 0


# read the partially written grid
reader SetFileName $file2
reader Update

vtkContourFilter cF2
  cF2 SetInputConnection [reader GetOutputPort]
  cF2 SetValue 0 0.38

vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [cF2 GetOutputPort]
  mapper2 ScalarVisibilityOff

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 0 10 0

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
