# Demonstrates the use of surface reconstruction
catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Read some points. Use a programmable filter to read them.
#
vtkProgrammableSource source
    source SetExecuteMethod readPoints
proc readPoints {} {
    set output [source GetPolyDataOutput]
    vtkPoints points
    $output SetPoints points

   set file [open "../../../vtkdata/SampledPoints/club71.16864.pts" r]
#   set file [open "../../../vtkdata/SampledPoints/cactus.3337.pts" r]
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
    surf SetInput [source GetPolyDataOutput]
    surf DebugOn
vtkContourFilter cf
    cf SetInput [surf GetOutput]
    cf SetValue 0 0.0
vtkPolyDataMapper map
    map SetInput [cf GetOutput]
vtkActor surfaceActor
    surfaceActor SetMapper map
    [surfaceActor GetProperty] SetColor 1 0 0

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
renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

renWin SetFileName reconstructionSurface.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


