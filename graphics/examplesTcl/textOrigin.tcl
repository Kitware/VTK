catch {load vtktcl}
# Created oriented text
source ../../examplesTcl/vtkInt.tcl

# pipeline
vtkAxes axes
    axes SetOrigin 5000 5000 5000
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
    textActor SetOrigin 3 0.5 0
#    textActor SetScale 0.2 0.2 0.2
    textActor SetPosition 5000 5000 5000
    textActor AddPosition -3 -0.5 0

# create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor axesActor
ren1 AddActor textActor
#[ren1 GetActiveCamera] Elevation 20
renWin Render
textActor SetCamera [ren1 GetActiveCamera]

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize


renWin SetFileName "textOrigin.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .
