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

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

[[[pl3d GetOutput] GetPointData] GetActiveVectors] SetName vectors

vtkExtractVectorComponents extract
  extract SetInput [pl3d GetOutput]
  extract ExtractToFieldDataOn
vtkContourFilter iso
    iso SetInput [extract GetOutput 0]
    iso SetValue 0 .38
vtkPolyDataNormals normals
    normals SetInput [iso GetOutput]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInput [normals GetOutput]
    isoMapper ImmediateModeRenderingOn
    isoMapper SetScalarModeToUsePointFieldData
    isoMapper ColorByArrayComponent vectors-z 0
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $tomato


# Add the actors to the renderer, set the background and size
#
ren1 AddActor isoActor
ren1 SetBackground .8 .8 .8
renWin SetSize 321 321

[ren1 GetActiveCamera] SetPosition -63.3093 -1.55444 64.3922
[ren1 GetActiveCamera] SetFocalPoint 8.255 0.0499763 29.7631
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName "extractVectors.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


