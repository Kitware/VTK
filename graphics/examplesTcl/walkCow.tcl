catch {load vtktcl}
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

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
  cow SetGeometryFileName "../../../vtkdata/Viewpoint/cow.g"

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
renWin SetSize 640 480
[ren1 GetActiveCamera] Azimuth 0
[ren1 GetActiveCamera] Dolly 1.4
iren Initialize
cowAxes VisibilityOn
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#
vtkTransform cowTransform

proc walk {} {
  cowActor SetOrientation 0 0 0
  cowActor SetOrigin 0 0 0
  cowActor SetPosition 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
   for {set i 1} {$i <= 6} {incr i} {
#	cowActor RotateY 60
  cowTransform Identity
  cowTransform RotateY [expr $i * 60]
  cowTransform Translate 0 0 5
        cowActor SetUserMatrix [cowTransform GetMatrix]
        renWin Render
        renWin Render
    }
  renWin EraseOn
}


proc walk2 {} {
  cowActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
  cowActor SetOrigin 0 0 -5
  cowActor SetPosition 0 0 5
  cowTransform Identity
  cowActor SetUserMatrix [cowTransform GetMatrix]
  for {set i 1} {$i <= 6} {incr i} {
    cowActor RotateY 60
    renWin Render
    renWin Render
  }
  renWin EraseOn
}

proc walk3 {} {
  cowActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
  cowActor SetOrigin 0 0 -5
  cowActor SetPosition 0 0 0
  cowTransform Identity
  cowActor SetUserMatrix [cowTransform GetMatrix]
  for {set i 1} {$i <= 6} {incr i} {
    cowActor RotateY 60
    renWin Render
    renWin Render
  }
  renWin EraseOn
}

proc walk4 {} {
  cowActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
  cowActor SetOrigin 6.11414 1.27386 .015175
  cowActor SetOrigin 0 0 0
  cowActor SetPosition 0 0 0
  cowTransform Identity
  cowActor SetUserMatrix [cowTransform GetMatrix]
  for {set i 1} {$i <= 6} {incr i} {
    cowActor RotateWXYZ 60 2.19574 -1.42455 -.0331036
    renWin Render
    renWin Render
  }
  renWin EraseOn
}


walk4
renWin EraseOff

#renWin SetFileName "walkCow.tcl.ppm"
#renWin SaveImageAsPPM

