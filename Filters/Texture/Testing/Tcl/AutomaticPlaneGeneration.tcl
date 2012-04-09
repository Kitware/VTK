package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPlaneSource aPlane
  aPlane SetCenter -100 -100 -100
  aPlane SetOrigin -100 -100 -100
  aPlane SetPoint1  -90 -100 -100
  aPlane SetPoint2  -100 -90 -100
  aPlane SetNormal  0 -1 1

vtkPNMReader imageIn
  imageIn SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkTexture texture
  texture SetInputConnection [imageIn GetOutputPort]

vtkTextureMapToPlane texturePlane
  texturePlane SetInputConnection [aPlane GetOutputPort]
  texturePlane AutomaticPlaneGenerationOn

vtkPolyDataMapper planeMapper
  planeMapper SetInputConnection [texturePlane GetOutputPort]

vtkActor texturedPlane
  texturedPlane SetMapper planeMapper
  texturedPlane SetTexture texture

# Add the actors to the renderer, set the background and size
#
ren1 AddActor texturedPlane
#ren1 SetBackground 1 1 1
renWin SetSize 200 200
renWin Render

renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


