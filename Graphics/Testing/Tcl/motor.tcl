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

# create cutting planes
vtkPlanes planes
vtkPoints points
vtkFloatArray norms
norms SetNumberOfComponents 3

points InsertPoint 0 0.0 0.0 0.0
norms InsertTuple3 0 0.0 0.0 1.0;    
points InsertPoint 1 0.0 0.0 0.0
norms InsertTuple3 1 -1.0 0.0 0.0;    

planes SetPoints points
planes SetNormals norms

# texture
vtkStructuredPointsReader texReader
    texReader SetFileName "$VTK_DATA_ROOT/Data/texThres2.vtk"
vtkTexture texture
    texture SetInputConnection [texReader GetOutputPort]
    texture InterpolateOff
    texture RepeatOff

# read motor parts...each part colored separately
#
vtkBYUReader byu
    byu SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu SetPartNumber 1
vtkPolyDataNormals normals
    normals SetInputConnection [byu GetOutputPort]
vtkImplicitTextureCoords tex1
    tex1 SetInputConnection [normals GetOutputPort]
    tex1 SetRFunction planes
#    tex1 FlipTextureOn
vtkDataSetMapper byuMapper
    byuMapper SetInputConnection [tex1 GetOutputPort]
vtkActor byuActor
    byuActor SetMapper byuMapper
    byuActor SetTexture texture
    eval [byuActor GetProperty] SetColor $cold_grey

vtkBYUReader byu2
    byu2 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu2 SetPartNumber 2
vtkPolyDataNormals normals2
    normals2 SetInputConnection [byu2 GetOutputPort]
vtkImplicitTextureCoords tex2
    tex2 SetInputConnection [normals2 GetOutputPort]
    tex2 SetRFunction planes
#    tex2 FlipTextureOn
vtkDataSetMapper byuMapper2
    byuMapper2 SetInputConnection [tex2 GetOutputPort]
vtkActor byuActor2
    byuActor2 SetMapper byuMapper2
    byuActor2 SetTexture texture
    eval [byuActor2 GetProperty] SetColor $peacock

vtkBYUReader byu3
    byu3 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu3 SetPartNumber 3

vtkTriangleFilter triangle3
  triangle3 SetInputConnection [byu3 GetOutputPort]

vtkPolyDataNormals normals3
    normals3 SetInputConnection [triangle3 GetOutputPort]
vtkImplicitTextureCoords tex3
    tex3 SetInputConnection [normals3 GetOutputPort]
    tex3 SetRFunction planes
#    tex3 FlipTextureOn
vtkDataSetMapper byuMapper3
    byuMapper3 SetInputConnection [tex3 GetOutputPort]
vtkActor byuActor3
    byuActor3 SetMapper byuMapper3
    byuActor3 SetTexture texture
    eval [byuActor3 GetProperty] SetColor $raw_sienna

vtkBYUReader byu4
    byu4 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu4 SetPartNumber 4
vtkPolyDataNormals normals4
    normals4 SetInputConnection [byu4 GetOutputPort]
vtkImplicitTextureCoords tex4
    tex4 SetInputConnection [normals4 GetOutputPort]
    tex4 SetRFunction planes
#    tex4 FlipTextureOn
vtkDataSetMapper byuMapper4
    byuMapper4 SetInputConnection [tex4 GetOutputPort]
vtkActor byuActor4
    byuActor4 SetMapper byuMapper4
    byuActor4 SetTexture texture
    eval [byuActor4 GetProperty] SetColor $banana

vtkBYUReader byu5
    byu5 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu5 SetPartNumber 5
vtkPolyDataNormals normals5
    normals5 SetInputConnection [byu5 GetOutputPort]
vtkImplicitTextureCoords tex5
    tex5 SetInputConnection [normals5 GetOutputPort]
    tex5 SetRFunction planes
#    tex5 FlipTextureOn
vtkDataSetMapper byuMapper5
    byuMapper5 SetInputConnection [tex5 GetOutputPort]
vtkActor byuActor5
    byuActor5 SetMapper byuMapper5
    byuActor5 SetTexture texture
    eval [byuActor5 GetProperty] SetColor $peach_puff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor byuActor
ren1 AddActor byuActor2
ren1 AddActor byuActor3
byuActor3 VisibilityOff
ren1 AddActor byuActor4
ren1 AddActor byuActor5
ren1 SetBackground 1 1 1
renWin SetSize 300 300

vtkCamera camera
    camera SetFocalPoint 0.0286334 0.0362996 0.0379685
    camera SetPosition 1.37067 1.08629 -1.30349
    camera SetViewAngle 17.673
    camera SetClippingRange 1 10
    camera SetViewUp  -0.376306 -0.5085 -0.774482

ren1 SetActiveCamera camera

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

