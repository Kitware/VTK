package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
# make sure to have the same regression image on all platforms.
    renWin SetMultiSamples 0

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Force a starting random value
vtkMath raMath
raMath RandomSeed 6

# Generate random attributes on a plane
#
vtkPlaneSource ps
ps SetXResolution 10
ps SetYResolution 10

vtkRandomAttributeGenerator ag
ag SetInputConnection [ps GetOutputPort]
ag GenerateAllDataOn

vtkSphereSource ss
ss SetPhiResolution 16
ss SetThetaResolution 32

vtkTensorGlyph tg
tg SetInputConnection [ag GetOutputPort]
tg SetSourceConnection [ss GetOutputPort]
tg SetScaleFactor 0.1
tg SetMaxScaleFactor 10
tg ClampScalingOn

vtkPolyDataNormals n
n SetInputConnection [tg GetOutputPort]

vtkConeSource cs
cs SetResolution 6

vtkGlyph3D glyph
glyph SetInputConnection [ag GetOutputPort]
glyph SetSourceConnection [cs GetOutputPort]
glyph SetScaleModeToDataScalingOff
glyph SetScaleFactor 0.05

vtkPolyDataMapper pdm
pdm SetInputConnection [n GetOutputPort]
#pdm SetInputConnection [glyph GetOutputPort]

vtkActor a
a SetMapper pdm

vtkPolyDataMapper pm
pm SetInputConnection [ps GetOutputPort]

vtkActor pa
pa SetMapper pm

ren1 AddActor a
ren1 AddActor pa
ren1 SetBackground 0 0 0

renWin SetSize 300 300
renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
