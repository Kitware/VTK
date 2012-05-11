package require vtk
package require vtkinteraction


# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin



#
# If the current directory is writable, then test the witers
#

if {[catch {set channel [open "test.tmp" "w"]}] == 0 } {
   close $channel
   file delete -force "test.tmp"

  # ====== Structured Grid ======
  # First save out a grid in parallel form.
  vtkMultiBlockPLOT3DReader reader
    reader SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    reader SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    reader Update
  vtkPDataSetWriter writer
    writer SetFileName "comb.pvtk"
    writer SetInputData [[reader GetOutput] GetBlock 0]
    writer SetNumberOfPieces 4
    writer Write
  vtkPDataSetReader pReader
    pReader SetFileName "comb.pvtk"
  vtkDataSetSurfaceFilter surface
    surface SetInputConnection [pReader GetOutputPort]
  vtkPolyDataMapper mapper
    mapper SetInputConnection [surface GetOutputPort]
    mapper SetNumberOfPieces 2
    mapper SetPiece 0
    mapper SetGhostLevel 1
    mapper Update

  file delete -force "comb.pvtk"
  file delete -force "comb.0.vtk"
  file delete -force "comb.1.vtk"
  file delete -force "comb.2.vtk"
  file delete -force "comb.3.vtk"

  vtkActor actor
    actor SetMapper mapper
    actor SetPosition -5 0 -29

  # Add the actors to the renderer, set the background and size
  #
  ren1 AddActor actor



  # ====== ImageData ======
  # First save out a grid in parallel form.
  vtkImageMandelbrotSource fractal
    fractal SetWholeExtent 0 9 0 9 0 9
    fractal SetSampleCX 0.1 0.1 0.1 0.1
    fractal SetMaximumNumberOfIterations 10

  vtkPDataSetWriter writer2
    writer SetFileName "fractal.pvtk"
    writer SetInputConnection [fractal GetOutputPort]
    writer SetNumberOfPieces 4
    writer Write

  vtkPDataSetReader pReader2
    pReader2 SetFileName "fractal.pvtk"

  vtkContourFilter iso
    iso SetInputConnection [pReader2 GetOutputPort]
    iso SetValue 0 4

  vtkPolyDataMapper mapper2
    mapper2 SetInputConnection [iso GetOutputPort]
    mapper2 SetNumberOfPieces 3
    mapper2 SetPiece 0
    mapper2 SetGhostLevel 0
    mapper2 Update

  file delete -force "fractal.pvtk"
  file delete -force "fractal.0.vtk"
  file delete -force "fractal.1.vtk"
  file delete -force "fractal.2.vtk"
  file delete -force "fractal.3.vtk"

  vtkActor actor2
    actor2 SetMapper mapper2
    actor2 SetScale 5 5 5
    actor2 SetPosition 6 6 6

  # Add the actors to the renderer, set the background and size
  #
  ren1 AddActor actor2



  # ====== PolyData ======
  # First save out a grid in parallel form.
  vtkSphereSource sphere
    sphere SetRadius 2

  vtkPDataSetWriter writer3
    writer3 SetFileName "sphere.pvtk"
    writer3 SetInputConnection [sphere GetOutputPort]
    writer3 SetNumberOfPieces 4
    writer3 Write

  vtkPDataSetReader pReader3
    pReader3 SetFileName "sphere.pvtk"

  vtkPolyDataMapper mapper3
    mapper3 SetInputConnection [pReader3 GetOutputPort]
    mapper3 SetNumberOfPieces 2
    mapper3 SetPiece 0
    mapper3 SetGhostLevel 1
    mapper3 Update

  file delete -force "sphere.pvtk"
  file delete -force "sphere.0.vtk"
  file delete -force "sphere.1.vtk"
  file delete -force "sphere.2.vtk"
  file delete -force "sphere.3.vtk"

  vtkActor actor3
    actor3 SetMapper mapper3
    actor3 SetPosition 6 6 6

  # Add the actors to the renderer, set the background and size
  #
  ren1 AddActor actor3
}

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 20
$cam1 Elevation 40
ren1 ResetCamera
$cam1 Zoom 1.2

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



