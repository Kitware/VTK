package require vtk
package require vtkinteraction

# this is a tcl version of plate vibration

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkPolyDataReader plate
    plate SetFileName "$VTK_DATA_ROOT/Data/plate.vtk"
    plate SetVectorsName "mode8"
vtkWarpVector warp
    warp SetInput [plate GetOutput]
    warp SetScaleFactor 0.5
vtkPolyDataNormals normals
    normals SetInput [warp GetPolyDataOutput]
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
renWin SetSize 250 250

[ren1 GetActiveCamera] SetPosition 13.3991 14.0764 9.97787 
[ren1 GetActiveCamera] SetFocalPoint 1.50437 0.481517 4.52992 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp -0.120861 0.458556 -0.880408 
[ren1 GetActiveCamera] SetClippingRange 12.5724 26.8374 

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
