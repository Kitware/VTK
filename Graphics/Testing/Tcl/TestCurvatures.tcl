package require vtk
package require vtkinteraction

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Pipeline stuff
#
vtkSuperquadricSource torus
    torus SetCenter 0.0 0.0 0.0
    torus SetScale 1.0 1.0 1.0
    torus SetPhiResolution 64
    torus SetThetaResolution 64
    torus SetPhiRoundness 0.5
    torus SetThetaRoundness 0.5
    torus SetThickness 0.5
    torus SetSize 0.5
    torus SetToroidal 1

# The quadric is made of strips, so pass it through a triangle filter as
# the curvature filter only operates on polys
vtkTriangleFilter tri
    tri SetInput [torus GetOutput]

# The quadric has nasty discontinuities from the way the edges are generated
# so let's pass it though a CleanPolyDataFilter and merge any points which
# are coincident, or very close

vtkCleanPolyData cleaner
    cleaner SetInput [tri GetOutput]
    cleaner SetTolerance 0.005

vtkCurvatures curve1
    curve1 SetInput [cleaner GetOutput]
    curve1 SetCurvatureTypeToGaussian

vtkCurvatures curve2
    curve2 SetInput [cleaner GetOutput]
    curve2 SetCurvatureTypeToMean

vtkLookupTable lut
    lut SetNumberOfColors 256
    lut SetHueRange 0.15 1.0
    lut SetSaturationRange 1.0 1.0
    lut SetValueRange 1.0 1.0
    lut SetAlphaRange 1.0 1.0
    lut SetRange -20 20

vtkPolyDataMapper cmapper1
    cmapper1 SetInput [curve1 GetOutput]
    cmapper1 SetLookupTable lut
    cmapper1 SetUseLookupTableScalarRange 1

vtkPolyDataMapper cmapper2
    cmapper2 SetInput [curve2 GetOutput]
    cmapper2 SetLookupTable lut
    cmapper2 SetUseLookupTableScalarRange 1

vtkActor cActor1
    cActor1 SetMapper cmapper1
    cActor1 SetPosition -0.5 0.0 0.0

vtkActor cActor2
    cActor2 SetMapper cmapper2
    cActor2 SetPosition  0.5 0.0 0.0

# Add the actors to the renderer
#
ren1 AddActor cActor1
ren1 AddActor cActor2

ren1 SetBackground 0.5 0.5 0.5
renWin SetSize 300 200

vtkCamera camera
ren1 SetActiveCamera camera

camera SetPosition 0.0 2.0 2.1
camera SetFocalPoint 0.0 0.0 0.0
camera SetViewAngle 30

ren1 ResetCameraClippingRange

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .


renWin Render
