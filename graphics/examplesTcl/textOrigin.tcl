catch {load vtktcl}
# Created oriented text
source ../../examplesTcl/vtkInt.tcl

# pipeline
vtkAxes axes
    axes SetOrigin 1000000 1000000 1000000
vtkPolyDataMapper axesMapper
    axesMapper SetInput [axes GetOutput]
vtkActor axesActor
    axesActor SetMapper axesMapper

vtkVectorText atext
    atext SetText "Origin"
vtkPolyDataMapper textMapper
    textMapper SetInput [atext GetOutput]
vtkFollower textActor
    textActor SetMapper textMapper
    textActor SetScale 0.2 0.2 0.2
    textActor AddPosition 0 -0.1 0

# create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor axesActor

#ren1 AddActor textActor
[ren1 GetActiveCamera] SetViewAngle 90
renWin Render
textActor SetCamera [ren1 GetActiveCamera]

#iren SetUserMethod {wm deiconify .vtkInteract}
#iren Initialize

ren1 SetStartRenderMethod {puts [[[ren1 GetActiveCamera] GetCompositePerspectiveTransformMatrix 1.0 0.0 1.0] Print]}
for {set i 0} {$i < 100} {incr i} {
   renWin Render
#   [ren1 GetActiveCamera] Azimuth 1
   [ren1 GetActiveCamera] SetPosition 1000000 [expr 1000000 + $i/100.0] 1000002
   [ren1 GetActiveCamera] SetFocalPoint 1000000 [expr 1000000 + $i/100.0] 1000000
}
for {set i 0} {$i < 100} {incr i} {
   renWin Render
#   [ren1 GetActiveCamera] Azimuth 1
   [ren1 GetActiveCamera] SetPosition [expr 1000000 + $i/100.0] 1000000 1000002
   [ren1 GetActiveCamera] SetFocalPoint [expr 1000000 + $i/100.0] 1000000 1000000
   puts [expr 1000000 + $i/100.0]
   puts [[ren1 GetActiveCamera] Print]
}
exit

renWin SetFileName "textOrigin.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .
