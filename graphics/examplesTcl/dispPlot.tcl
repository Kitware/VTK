catch {load vtktcl}
catch {load vtktcl}
# this is a tcl version of plate vibration
# get the interactor ui
source vtkInt.tcl
# First create the render master
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# read a vtk file
#
vtkPolyReader plate
    plate SetFileName "../../../data/plate.vtk"
    plate SetVectorsName "mode8"
vtkWarpVector warp
    warp SetInput [plate GetOutput]
    warp SetScaleFactor 0.5
vtkGeometryFilter ds2poly
    ds2poly SetInput [warp GetOutput]
vtkCleanPolyData clean
    clean SetInput [ds2poly GetOutput]
vtkPolyNormals normals
    normals SetInput [clean GetOutput]
vtkVectorDot color
    color SetInput [normals GetOutput]
vtkLookupTable lut
    lut SetNumberOfColors 256
    lut Build
    for {set i 0} {$i<128} {incr i 1} {
        eval lut SetTableValue $i [expr (128.0-$i)/128.0] [expr (128.0-$i)/128.0] [expr (128.0-$i)/128.0] 1
    }
    for {set i 128} {$i<256} {incr i 1} {
        eval lut SetTableValue $i [expr ($i-128.0)/128.0] [expr ($i-128.0)/128.0] [expr ($i-128.0)/128.0] 1
    }

vtkDataSetMapper plateMapper
    plateMapper SetInput [color GetOutput]
    plateMapper SetLookupTable lut
    plateMapper SetScalarRange -1 1
vtkActor plateActor
    plateActor SetMapper plateMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors plateActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize

#$renWin SetFileName "dispPlot.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


