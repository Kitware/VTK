package require vtk
package require vtkinteraction

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkBYUReader cow
  cow SetGeometryFileName "$VTK_DATA_ROOT/Data/Viewpoint/cow.g"

vtkPolyDataMapper cowMapper
    cowMapper SetInput [cow GetOutput]
vtkActor cowActor
    cowActor SetMapper cowMapper
    [cowActor GetProperty] SetDiffuseColor 0.9608 0.8706 0.7020

vtkAxes cowAxesSource
  cowAxesSource SetScaleFactor 10
  cowAxesSource SetOrigin 0 0 0

vtkPolyDataMapper cowAxesMapper
  cowAxesMapper SetInput [cowAxesSource GetOutput]
 
vtkActor cowAxes
  cowAxes SetMapper cowAxesMapper

ren1 AddActor cowAxes
cowAxes VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cowActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 320 240
[ren1 GetActiveCamera] Azimuth 0
[ren1 GetActiveCamera] Dolly 1.4
ren1 ResetCameraClippingRange

cowAxes VisibilityOn
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .

#
proc RotateX {} {
  cowActor SetOrientation 0 0 0
  ren1 ResetCameraClippingRange
  renWin Render
  renWin Render
  renWin EraseOff
   for {set i 1} {$i <= 6} {incr i} {
	cowActor RotateX 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateY {} {
  cowActor SetOrientation 0 0 0
  ren1 ResetCameraClippingRange
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	cowActor RotateY 60 
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateZ {} {
  cowActor SetOrientation 0 0 0
  ren1 ResetCameraClippingRange
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	cowActor RotateZ 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateXY {} {
  cowActor SetOrientation 0 0 0
  cowActor RotateX 60
  ren1 ResetCameraClippingRange
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	cowActor RotateY 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}

RotateX
RotateY
RotateZ
RotateXY
renWin EraseOff
