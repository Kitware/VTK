catch {load vtktcl}
# this is a tcl version of plate vibration
# get the interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkPolyDataReader plate
    plate SetFileName "../../../data/plate.vtk"
    plate SetVectorsName "mode8"
vtkWarpVector warp
    warp SetInput [plate GetOutput]
    warp SetScaleFactor 0.5
vtkCastToConcrete caster
    caster SetInput [warp GetOutput]
vtkPolyDataNormals normals
    normals SetInput [caster GetPolyDataOutput]
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
ren1 AddActor plateActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "dispPlot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


