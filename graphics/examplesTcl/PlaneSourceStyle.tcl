catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

vtkInteractorStylePlane planeStyle
set planeSource [planeStyle GetPlaneSource]
$planeSource SetXResolution 50
$planeSource SetYResolution 50
$planeSource SetOrigin 2.487 -3.188 25.5
$planeSource SetPoint1 12.25 -3.87 30.1
$planeSource SetPoint2 0.345 -3.81 30.0

# 0 16.51   -5.662  5.662    23.331 36.195

vtkProbeFilter probe
    probe SetInput [$planeSource GetOutput]
    probe SetSource [pl3d GetOutput]
vtkDataSetMapper cutMapper
    cutMapper SetInput [probe GetOutput]
    eval cutMapper SetScalarRange 0.198 0.4
        # 0.198 0.710
vtkActor cutActor
    cutActor SetMapper cutMapper
    [cutActor GetProperty] SetAmbient 1.0
    [cutActor GetProperty] SetDiffuse 0.0

#extract plane
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInput [pl3d GetOutput]
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyDataMapper planeMapper
    planeMapper SetInput [compPlane GetOutput]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 AddActor cutActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876
iren Initialize
iren SetInteractorStyle planeStyle

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "probe.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



