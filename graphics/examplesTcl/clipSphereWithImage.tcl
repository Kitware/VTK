catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
# and some nice colors
source ../../examplesTcl/colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPNMReader image
  image SetFileName "../../../vtkdata/vtks.pgm"
  image SetDataOrigin -.5 [expr -(160.0/320.00) / 2.0] 0
  image SetDataSpacing [expr 1.0 / 320.00] [ expr 1.0 / 320.00] 1
  image Update

vtkImageGaussianSmooth gaussian
    gaussian SetInput [image GetOutput]
    eval gaussian SetStandardDeviations 5 5
    gaussian SetDimensionality 2
    gaussian SetRadiusFactors 2 2

vtkImageToStructuredPoints toStructuredPoints
    toStructuredPoints SetInput [gaussian GetOutput]
    toStructuredPoints Update

vtkTransform transform
  transform Identity
  transform Scale .75 .75 .75
  transform Inverse
  transform Scale 1 1 0

vtkImplicitVolume aVolume
  aVolume SetVolume [toStructuredPoints GetOutput]
  aVolume SetTransform transform
  aVolume SetOutValue 256

vtkSphereSource aSphere
  aSphere SetPhiResolution 200
  aSphere SetThetaResolution 200

vtkClipPolyData aClipper
    aClipper SetInput [aSphere GetOutput]
    aClipper SetValue 127.5
    aClipper GenerateClipScalarsOn
    aClipper SetClipFunction aVolume
    aClipper Update

vtkPolyDataMapper mapper
  mapper SetInput [aClipper GetOutput]
  mapper ScalarVisibilityOff

vtkProperty backProp
  eval  backProp SetDiffuseColor $tomato

vtkActor sphereActor
  sphereActor SetMapper mapper
  sphereActor SetBackfaceProperty backProp
eval [sphereActor GetProperty] SetDiffuseColor $banana
ren1 AddActor sphereActor

[ren1 GetActiveCamera] Azimuth -20
[ren1 GetActiveCamera] Elevation 15
[ren1 GetActiveCamera] Dolly 1.2

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 320 320

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName clipSphereWithImage.tcl.ppm
#renWin SaveImageAsPPM


# prevent the tk window from showing up then start the event loop
wm withdraw .
