# This example shows how to construct a surface from a point cloud. First
# we generate a volume using the vtkSurfaceReconstructionFilter. The volume
# values are a distance field. Once this is generated, the volume is 
# countoured at a distance value of 0.0.
#
package require vtk
package require vtkinteraction

# Read some points. Use a programmable filter to read them.
#
vtkProgrammableSource pointSource
    pointSource SetExecuteMethod readPoints

proc readPoints {} {
global VTK_DATA_ROOT
    set output [pointSource GetPolyDataOutput]
    vtkPoints points
    $output SetPoints points

   set file [open "$VTK_DATA_ROOT/Data/cactus.3337.pts" r]
   while { [gets $file line] != -1 } {
      scan $line "%s" firstToken
      if { $firstToken == "p" } {
         scan $line "%s %f %f %f" firstToken x y z
         points InsertNextPoint $x $y $z
      }
   }
   points Delete; #okay, reference counting
}

# Construct the surface and create isosurface.
#
vtkSurfaceReconstructionFilter surf
    surf SetInputConnection [pointSource GetOutputPort]

vtkContourFilter cf
    cf SetInputConnection [surf GetOutputPort]
    cf SetValue 0 0.0

# Sometimes the contouring algorithm can create a volume whose gradient
# vector and ordering of polygon (using the right hand rule) are 
# inconsistent. vtkReverseSense cures this problem.
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
renWin SetSize 400 400
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


