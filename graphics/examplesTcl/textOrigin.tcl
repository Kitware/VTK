catch {load vtktcl}
# Created oriented text
source vtkInt.tcl

# pipeline
vtkAxes axes
    axes SetOrigin 0 0 0
vtkPolyMapper axesMapper
    axesMapper SetInput [axes GetOutput]
vtkActor axesActor
    axesActor SetMapper axesMapper

vtkVectorText atext
    atext SetText "Origin"
vtkPolyMapper textMapper
    textMapper SetInput [atext GetOutput]
vtkFollower textActor
    textActor SetMapper textMapper
    textActor SetScale 0.2 0.2 0.2
    textActor AddPosition 0 -0.1 0

# create graphics stuff
vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

$ren1 AddActor axesActor
$ren1 AddActor textActor
[$ren1 GetActiveCamera] Zoom 1.6
$renWin Render
textActor SetCamera [$ren1 GetActiveCamera]

$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize

$renWin SetFileName "textOrigin.tcl.ppm"
#$renWin SaveImageAsPPM

wm withdraw .
