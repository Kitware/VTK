catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and Interactor
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

vtkExtractGrid extract
  extract SetVOI 1 55 -1000 1000 -1000 1000
  extract SetInput [pl3d GetOutput]

vtkPlane plane
  plane SetOrigin 0 4 2
  plane SetNormal 0 1 0
vtkCutter cutter
  cutter SetInput [extract GetOutput]
  cutter SetCutFunction plane
  cutter GenerateCutScalarsOff
  cutter SetSortBy 1

vtkLookupTable clut
  clut SetHueRange 0 .67
  clut Build

vtkPolyDataMapper cutterMapper
cutterMapper SetInput [cutter GetOutput]
cutterMapper SetScalarRange .18 .7
cutterMapper SetLookupTable clut

vtkActor cut
  cut SetMapper cutterMapper

vtkContourFilter iso
    iso SetInput [pl3d GetOutput]
    iso SetValue 0 .22
vtkPolyDataNormals normals
    normals SetInput [iso GetOutput]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInput [normals GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetDiffuseColor $tomato
    eval [isoActor GetProperty] SetSpecularColor $white
    eval [isoActor GetProperty] SetDiffuse .8
    eval [isoActor GetProperty] SetSpecular .5
    eval [isoActor GetProperty] SetSpecularPower 30

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkTubeFilter outlineTubes
  outlineTubes SetInput [outline GetOutput]
  outlineTubes SetRadius .1

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outlineTubes GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
eval [outlineActor GetProperty] SetColor $banana
ren1 AddActor isoActor
isoActor VisibilityOn
ren1 AddActor cut
set opacity .06
[cut GetProperty] SetOpacity 1
ren1 SetBackground 1 1 1
renWin SetSize 640 480

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#
# Cut: generates n cut planes normal to camera's view plane
#
proc Cut {n} {
  global cam1 opacity
  eval plane SetNormal [$cam1 GetViewPlaneNormal]
  eval plane SetOrigin [$cam1 GetFocalPoint]
  eval cutter GenerateValues $n -15 15
  clut SetAlphaRange $opacity $opacity
  renWin Render
}

# Generate 10 cut planes
Cut 10
#renWin SetFileName "valid/combVol.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
