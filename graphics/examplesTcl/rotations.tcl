catch {load vtktcl}
# Generate implicit model of a sphere
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
vtkBYUReader cone
  cone SetGeometryFileName "../../../vtkdata/Viewpoint/cow.g"

vtkPolyDataMapper coneMapper
    coneMapper SetInput [cone GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMapper
    [coneActor GetProperty] SetDiffuseColor 0.9608 0.8706 0.7020

vtkAxes coneAxesSource
  coneAxesSource SetScaleFactor 10
  coneAxesSource SetOrigin 0 0 0

vtkPolyDataMapper coneAxesMapper
  coneAxesMapper SetInput [coneAxesSource GetOutput]
 
vtkActor coneAxes
  coneAxes SetMapper coneAxesMapper

ren1 AddActor coneAxes
coneAxes VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4
#renWin SetSize 1280 1024
renWin SetSize 640 480
[ren1 GetActiveCamera] Azimuth 0
[ren1 GetActiveCamera] Dolly 1.4
iren Initialize
coneAxes VisibilityOn
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .

#
proc RotateX {} {
  coneActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
   for {set i 1} {$i <= 6} {incr i} {
	coneActor RotateX 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateY {} {
  coneActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	coneActor RotateY 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateZ {} {
  coneActor SetOrientation 0 0 0
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	coneActor RotateZ 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}
proc RotateXY {} {
  coneActor SetOrientation 0 0 0
  coneActor RotateX 60
  renWin Render
  renWin Render
  renWin EraseOff
    for {set i 1} {$i <= 6} {incr i} {
	coneActor RotateY 60
        renWin Render
        renWin Render
    }
  renWin EraseOn
}

RotateX
renWin SetFileName rotX.ppm
#renWin SaveImageAsPPM

RotateY
renWin SetFileName rotY.ppm
#renWin SaveImageAsPPM

RotateZ
renWin SetFileName rotZ.ppm
#renWin SaveImageAsPPM

RotateXY
renWin EraseOff
renWin SetFileName rotXY.ppm
#renWin SaveImageAsPPM

renWin SetFileName "rotations.tcl.ppm"
renWin SaveImageAsPPM
