catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl
# and some nice colors
source $VTK_TCL/colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPNMReader imageIn
  imageIn SetFileName "$VTK_DATA/B.pgm"

vtkImageGaussianSmooth gaussian
    eval gaussian SetStandardDeviations 2 2
    gaussian SetDimensionality 2
    gaussian SetRadiusFactors 1 1
    gaussian SetInput [imageIn GetOutput]

vtkImageToStructuredPoints toStructuredPoints
    toStructuredPoints SetInput [gaussian GetOutput]

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [toStructuredPoints GetOutput]

vtkClipPolyData aClipper
    aClipper SetInput [geometry GetOutput]
    aClipper SetValue 127.5
    aClipper GenerateClipScalarsOff
    aClipper InsideOutOn
    [[aClipper GetOutput] GetPointData] CopyScalarsOff
    aClipper Update

vtkPolyDataMapper mapper
  mapper SetInput [aClipper GetOutput]
  mapper ScalarVisibilityOff

vtkActor letter
  letter SetMapper mapper

ren1 AddActor letter
  [letter GetProperty] SetDiffuseColor 0 0 0
  [letter GetProperty] SetRepresentationToWireframe

ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin SetSize 320 320
iren SetUserMethod {wm deiconify .vtkInteract}


# render the image
#
renWin Render
#renWin SetFileName "createBFont.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
