catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl
# and some nice colors
source colors.tcl

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPNMReader image
  image SetFileName "../../vtkdata/B.pgm"

vtkImageGaussianSmooth gaussian
    eval gaussian SetStandardDeviations 2 2
    gaussian SetDimensionality 2
    gaussian SetStrides 2 2
    gaussian SetRadiusFactors 1 1
    gaussian SetInput [image GetOutput]

vtkStructuredPointsGeometryFilter geometry
  geometry SetInput [gaussian GetOutput]

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
renWin SetSize 320 320

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# render the image
#
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
