package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


vtkTriangularTexture aTriangularTexture
    aTriangularTexture SetTexturePattern 2
    aTriangularTexture SetScaleFactor 1.3
    aTriangularTexture SetXSize 64
    aTriangularTexture SetYSize 64
  
vtkSphereSource aSphere
    aSphere SetThetaResolution 20
    aSphere SetPhiResolution 20

vtkTriangularTCoords tCoords
    tCoords SetInputConnection [aSphere GetOutputPort]

vtkPolyDataMapper triangleMapper
    triangleMapper SetInputConnection [tCoords GetOutputPort]

vtkTexture aTexture
    aTexture SetInputConnection [aTriangularTexture GetOutputPort]
    aTexture InterpolateOn

vtkActor texturedActor
    texturedActor SetMapper triangleMapper
    texturedActor SetTexture aTexture
    [texturedActor GetProperty] BackfaceCullingOn
    eval [texturedActor GetProperty] SetDiffuseColor $banana
    eval [texturedActor GetProperty] SetSpecular .4
    eval [texturedActor GetProperty] SetSpecularPower 40

vtkCubeSource aCube
    aCube SetXLength .5
    aCube SetYLength .5

vtkPolyDataMapper aCubeMapper
    aCubeMapper SetInputConnection [aCube GetOutputPort]

vtkActor cubeActor
    cubeActor SetMapper aCubeMapper
    eval [cubeActor GetProperty] SetDiffuseColor $tomato

eval ren1 SetBackground $slate_grey
ren1 AddActor cubeActor
ren1 AddActor texturedActor
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
