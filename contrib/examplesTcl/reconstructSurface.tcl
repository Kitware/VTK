# Demonstrates the use of surface reconstruction
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Read some points. Use a programmable filter to read them.
#
vtkProgrammableSource pointSource
    pointSource SetExecuteMethod readPoints
proc readPoints {} {
global VTK_DATA
    set output [pointSource GetPolyDataOutput]
    vtkPoints points
    $output SetPoints points

#   set file [open "$VTK_DATA/SampledPoints/club71.16864.pts" r]
   set file [open "$VTK_DATA/SampledPoints/cactus.3337.pts" r]
   while { [gets $file line] != -1 } {
      scan $line "%s" firstToken
      if { $firstToken == "p" } {
         scan $line "%s %f %f %f" firstToken x y z
         points InsertNextPoint $x $y $z
      }
   }
   points Delete; #okay, reference counting
}

# Construct the surface and create isosurface
#
vtkSurfaceReconstructionFilter surf
    surf SetInput [pointSource GetPolyDataOutput]

vtkContourFilter cf
    cf SetInput [surf GetOutput]
    cf SetValue 0 0.0

vtkReverseSense reverse
  reverse SetInput [cf GetOutput]
  reverse ReverseCellsOn
  reverse ReverseNormalsOn

vtkPolyDataMapper map
    map SetInput [reverse GetOutput]
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
renWin SetSize 500 500
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
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

renWin SetFileName reconstructSurface.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


