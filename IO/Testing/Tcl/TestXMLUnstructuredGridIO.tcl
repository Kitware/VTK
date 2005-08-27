package require vtk
package require vtkinteraction

set file0 ugFile0.vtu
set file1 ugFile1.vtu
set file2 ugFile2.vtu

# read in some unstructured grid data
vtkUnstructuredGridReader ugReader
  ugReader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
  ugReader SetScalarsName "thickness9"
  ugReader SetVectorsName "displacement9"

vtkExtractUnstructuredGridPiece extract
  extract SetInputConnection [ugReader GetOutputPort]

# write various versions 
vtkXMLUnstructuredGridWriter ugWriter
  ugWriter SetFileName $file0
  ugWriter SetDataModeToAscii
  ugWriter SetInputConnection [ugReader GetOutputPort]
  ugWriter Write
  
  ugWriter SetFileName $file1
  ugWriter SetInputConnection [extract GetOutputPort]
  ugWriter SetDataModeToAppended
  ugWriter SetNumberOfPieces 2
  ugWriter Write

  ugWriter SetFileName $file2
  ugWriter SetDataModeToBinary
  ugWriter SetGhostLevel 2
  ugWriter Write


# read the ASCII version
vtkXMLUnstructuredGridReader reader
  reader SetFileName $file0
  reader Update

vtkUnstructuredGrid ug0
  ug0 DeepCopy [reader GetOutput] 

vtkDataSetSurfaceFilter sF
  sF SetInput ug0

vtkPolyDataMapper mapper0
  mapper0 SetInputConnection [sF GetOutputPort]

vtkActor actor0
  actor0 SetMapper mapper0
  actor0 SetPosition 0 40 20


# read appended piece 0
reader SetFileName $file1

vtkDataSetSurfaceFilter sF1
  sF1 SetInputConnection [reader GetOutputPort]
  
vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [sF1 GetOutputPort]
  mapper1 SetPiece 1
  mapper1 SetNumberOfPieces 2

vtkActor actor1
  actor1 SetMapper mapper1


# read binary piece 0 (with ghost level)
vtkXMLUnstructuredGridReader reader2
  reader2 SetFileName $file2

vtkDataSetSurfaceFilter sF2
  sF2 SetInputConnection [reader2 GetOutputPort]
  
vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [sF2 GetOutputPort]
  mapper2 SetPiece 1
  mapper2 SetNumberOfPieces 2
  mapper2 SetGhostLevel 2

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 0 0 30

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

ren1 ResetCamera
[ren1 GetActiveCamera] SetPosition 180 55 65
[ren1 GetActiveCamera] SetFocalPoint 3.5 32 15 
renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

file delete -force $file0
file delete -force $file1
file delete -force $file2
