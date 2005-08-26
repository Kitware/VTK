package require vtk
package require vtkinteraction

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPNMReader imageIn
  imageIn SetFileName "$VTK_DATA_ROOT/Data/B.pgm"

vtkImageGaussianSmooth gaussian
    eval gaussian SetStandardDeviations 2 2
    gaussian SetDimensionality 2
    gaussian SetRadiusFactors 1 1
    gaussian SetInputConnection [imageIn GetOutputPort]

vtkImageDataGeometryFilter geometry
  geometry SetInputConnection [gaussian GetOutputPort]

vtkClipPolyData aClipper
    aClipper SetInputConnection [geometry GetOutputPort]
    aClipper SetValue 127.5
    aClipper GenerateClipScalarsOff
    aClipper InsideOutOn
    [[aClipper GetOutput] GetPointData] CopyScalarsOff
    aClipper Update

vtkPolyDataMapper mapper
  mapper SetInputConnection [aClipper GetOutputPort]
  mapper ScalarVisibilityOff

vtkActor letter
  letter SetMapper mapper

ren1 AddActor letter
  [letter GetProperty] SetDiffuseColor 0 0 0
  [letter GetProperty] SetRepresentationToWireframe

ren1 SetBackground 1 1 1
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin SetSize 320 320
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# render the image
#
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
