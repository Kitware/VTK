package require vtk
package require vtkinteraction

set file0 idFile0.vti
set file1 idFile1.vti
set file2 idFile2.vti

# read in some poly data
vtkPolyDataReader pdReader
  pdReader SetFileName "$VTK_DATA_ROOT/Data/fran_cut.vtk"
  pdReader Update

vtkExtractPolyDataPiece extract
  extract SetInputConnection [pdReader GetOutputPort]

# write various versions
vtkXMLPolyDataWriter pdWriter
  pdWriter SetFileName $file0
  pdWriter SetDataModeToAscii
  pdWriter SetInputConnection [pdReader GetOutputPort]
  pdWriter Write

  pdWriter SetFileName $file1
  pdWriter SetInputConnection [extract GetOutputPort]
  pdWriter SetDataModeToAppended
  pdWriter SetNumberOfPieces 2
  pdWriter Write

  pdWriter SetFileName $file2
  pdWriter SetDataModeToBinary
  pdWriter SetGhostLevel 3
  pdWriter Write


# read the ASCII version
vtkXMLPolyDataReader reader
  reader SetFileName $file0
  reader Update

vtkPolyData pd0
  pd0 DeepCopy [reader GetOutput]

vtkPolyDataMapper mapper0
  mapper0 SetInputData pd0

vtkActor actor0
  actor0 SetMapper mapper0
  actor0 SetPosition 0 .15 0


# read appended piece 0
reader SetFileName $file1

vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [reader GetOutputPort]
  mapper1 SetPiece 0
  mapper1 SetNumberOfPieces 2

vtkActor actor1
  actor1 SetMapper mapper1


# read binary piece 0 (with ghost level)
vtkXMLPolyDataReader reader2
  reader2 SetFileName $file2

vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [reader2 GetOutputPort]
  mapper2 SetPiece 0
  mapper2 SetNumberOfPieces 2

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetPosition 0 0 0.1

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

[ren1 GetActiveCamera] SetPosition 0.514096 -0.14323 -0.441177
[ren1 GetActiveCamera] SetFocalPoint 0.0528 -0.0780001 -0.0379661
renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

file delete -force $file0
file delete -force $file1
file delete -force $file2
