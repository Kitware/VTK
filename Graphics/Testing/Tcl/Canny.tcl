package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# load in the texture map
#
vtkPNMReader imageIn
imageIn SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkImageLuminance il
il SetInputConnection [imageIn GetOutputPort]

vtkImageCast ic
ic SetOutputScalarTypeToFloat
ic SetInputConnection [il GetOutputPort]

# smooth the image
vtkImageGaussianSmooth gs
gs SetInputConnection [ic GetOutputPort]
gs SetDimensionality 2
gs SetRadiusFactors 1 1 0

# gradient the image
vtkImageGradient imgGradient;
imgGradient SetInputConnection [gs GetOutputPort];
imgGradient SetDimensionality 2

vtkImageMagnitude imgMagnitude
imgMagnitude SetInputConnection [imgGradient GetOutputPort]

# non maximum suppression
vtkImageNonMaximumSuppression nonMax;
nonMax SetMagnitudeInput [imgMagnitude GetOutput];
nonMax SetVectorInput [imgGradient GetOutput];
nonMax SetDimensionality 2

vtkImageConstantPad pad
pad SetInputConnection [imgGradient GetOutputPort]
pad SetOutputNumberOfScalarComponents 3
pad SetConstant 0

vtkImageToStructuredPoints i2sp1
i2sp1 SetInputConnection [nonMax GetOutputPort]
i2sp1 SetVectorInput [pad GetOutput]

# link edgles
vtkLinkEdgels imgLink;
imgLink SetInputConnection [i2sp1 GetOutputPort];
imgLink SetGradientThreshold 2;

# threshold links
vtkThreshold thresholdEdgels;
thresholdEdgels SetInputConnection [imgLink GetOutputPort];
thresholdEdgels ThresholdByUpper 10;
thresholdEdgels AllScalarsOff

vtkGeometryFilter gf
gf SetInputConnection [thresholdEdgels GetOutputPort]

vtkImageToStructuredPoints i2sp
i2sp SetInputConnection [imgMagnitude GetOutputPort]
i2sp SetVectorInput [pad GetOutput]

# subpixel them
vtkSubPixelPositionEdgels spe;
spe SetInputConnection [gf GetOutputPort];
spe SetGradMaps [i2sp GetOutput];

vtkStripper strip
strip SetInputConnection [spe GetOutputPort]

vtkPolyDataMapper dsm
dsm SetInputConnection [strip GetOutputPort]
dsm ScalarVisibilityOff

vtkActor planeActor
planeActor SetMapper dsm
[planeActor GetProperty] SetAmbient 1.0
[planeActor GetProperty] SetDiffuse 0.0

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
renWin SetSize 600 300

# render the image
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render
[ren1 GetActiveCamera] Zoom 2.8
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





