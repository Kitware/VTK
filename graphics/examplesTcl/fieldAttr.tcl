# Test the attribute to field data filter to plot the y-ccomponents of a vector
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create pipeline
# Create an isosurface
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
vtkContourFilter iso
    iso SetInput [pl3d GetOutput]
    iso SetValue 0 .38

# Create a scalar that is the y-component of the vector and use it to
# color the isosurface.
vtkAttributeDataToFieldDataFilter ad2fd
    ad2fd SetInput [pl3d GetOutput]
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [ad2fd GetOutput]
    fd2ad SetInputFieldToPointDataField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetScalarComponent 0 PointVectors 1 
vtkProbeFilter probe
    probe SetInput [iso GetOutput]
    probe SetSource [fd2ad GetOutput]
vtkPolyDataNormals normals
    normals SetInput [probe GetPolyDataOutput]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInput [normals GetOutput]
    isoMapper ScalarVisibilityOn
    isoMapper SetScalarRange -25 350
vtkLODActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

# Grid outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
renWin SetFileName "fieldAttr.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


