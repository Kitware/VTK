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

if {[catch {set channel [open test.tmp w]}] == 0 } {
   close $channel
   file delete -force test.tmp

  # First save out a grid in parallel form.
  vtkPLOT3DReader reader
    reader SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    reader SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"

  vtkPDataSetWriter writer
    writer SetFileName "comb.pvtk"
    writer SetInput [reader GetOutput]
    writer SetNumberOfPieces 2
    writer Write

  vtkPDataSetReader pReader
    pReader SetFileName "comb.pvtk"
 
  vtkDataSetSurfaceFilter surface
    surface SetInput [pReader GetOutput]

  vtkPolyDataMapper mapper
    mapper SetInput [surface GetOutput]
    mapper SetNumberOfPieces 2
    mapper SetPiece 0
    mapper SetGhostLevel 1
    mapper Update


  file delete -force grid.pvtk
  file delete -force grid.0.vtk
  file delete -force grid.1.vtk

  vtkActor actor
    actor SetMapper mapper

  # Add the actors to the renderer, set the background and size
  #
  ren1 AddActor actor
}

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 20
$cam1 Elevation 40
ren1 ResetCamera
$cam1 Zoom 1.2

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



