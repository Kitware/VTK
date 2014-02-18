package require vtk
package require vtkinteraction
package require vtktesting

# Perform psuedo volume rendering in a structured grid by compositing
# translucent cut planes. This same trick can be used for unstructured
# grids. Note that for better results, more planes can be created. Also,
# if your data is vtkImageData, there are much faster methods for volume
# rendering.

# Create pipeline. Read structured grid data.
#
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

set pl3dOutput [[pl3d GetOutput] GetBlock 0]

# A convenience, use this filter to limit data for experimentation.
vtkExtractGrid extract
  extract SetVOI 1 55 -1000 1000 -1000 1000
  extract SetInputData $pl3dOutput

# The (implicit) plane is used to do the cutting
vtkPlane plane
  plane SetOrigin 0 4 2
  plane SetNormal 0 1 0

# The cutter is set up to process each contour value over all cells
# (SetSortByToSortByCell). This results in an ordered output of polygons
# which is key to the compositing.
vtkCutter cutter
  cutter SetInputConnection [extract GetOutputPort]
  cutter SetCutFunction plane
  cutter GenerateCutScalarsOff
  cutter SetSortByToSortByCell

vtkLookupTable clut
  clut SetHueRange 0 .67
  clut Build

vtkPolyDataMapper cutterMapper
cutterMapper SetInputConnection [cutter GetOutputPort]
cutterMapper SetScalarRange .18 .7
cutterMapper SetLookupTable clut

vtkActor cut
  cut SetMapper cutterMapper

# Add in some surface geometry for interest.
vtkContourFilter iso
    iso SetInputData $pl3dOutput
    iso SetValue 0 .22
vtkPolyDataNormals normals
    normals SetInputConnection [iso GetOutputPort]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInputConnection [normals GetOutputPort]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetDiffuseColor $tomato
    eval [isoActor GetProperty] SetSpecularColor $white
    eval [isoActor GetProperty] SetDiffuse .8
    eval [isoActor GetProperty] SetSpecular .5
    eval [isoActor GetProperty] SetSpecularPower 30

vtkStructuredGridOutlineFilter outline
    outline SetInputData $pl3dOutput
vtkTubeFilter outlineTubes
  outlineTubes SetInputConnection [outline GetOutputPort]
  outlineTubes SetRadius .1

vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outlineTubes GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
eval [outlineActor GetProperty] SetColor $banana
ren1 AddActor isoActor
isoActor VisibilityOn
ren1 AddActor cut
set opacity .1
[cut GetProperty] SetOpacity 1
ren1 SetBackground 1 1 1
renWin SetSize 640 480

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 ComputeViewPlaneNormal
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Cut: generates n cut planes normal to camera's view plane
#
proc Cut {n} {
  global cam1 opacity
  eval plane SetNormal [$cam1 GetViewPlaneNormal]
  eval plane SetOrigin [$cam1 GetFocalPoint]
  eval cutter GenerateValues $n -5 5
  clut SetAlphaRange $opacity $opacity
  renWin Render
}

# Generate 10 cut planes
Cut 20

# prevent the tk window from showing up then start the event loop
wm withdraw .
iren Start