package require vtk
package require vtkinteraction


proc readPoints {} {
global VTK_DATA_ROOT
    set output [pointSource GetPolyDataOutput]
    vtkPoints points
    $output SetPoints points

   set fp [open "$VTK_DATA_ROOT/Data/cactus.3337.pts" r]
   while { [gets $fp line] != -1 } {
      scan $line "%s" firstToken
      if { $firstToken == "p" } {
         scan $line "%s %f %f %f" firstToken x y z
         points InsertNextPoint $x $y $z
      }
   }
   points Delete; #okay, reference counting
}

# Read some points. Use a programmable filter to read them.
#
vtkProgrammableSource pointSource
    pointSource SetExecuteMethod readPoints

# Construct the surface and create isosurface
#
vtkSurfaceReconstructionFilter surf
    surf SetInput [pointSource GetPolyDataOutput]

vtkContourFilter cf
    cf SetInputConnection [surf GetOutputPort]
    cf SetValue 0 0.0

vtkReverseSense reverse
  reverse SetInputConnection [cf GetOutputPort]
  reverse ReverseCellsOn
  reverse ReverseNormalsOn

vtkPolyDataMapper map
    map SetInputConnection [reverse GetOutputPort]
    map ScalarVisibilityOff

vtkActor surfaceActor
    surfaceActor SetMapper map
    [surfaceActor GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784
    [surfaceActor GetProperty] SetSpecularColor 1 1 1
    [surfaceActor GetProperty] SetSpecular .4
    [surfaceActor GetProperty] SetSpecularPower 50

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor surfaceActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] SetPosition 1 0 0
[ren1 GetActiveCamera] SetViewUp 0 0 1
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 20
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


