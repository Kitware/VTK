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
    texture SetInput [texReader GetOutput]
    texture InterpolateOff
    texture RepeatOff

# read motor parts...each part colored separately
#
vtkBYUReader byu
    byu SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu SetPartNumber 1
vtkPolyDataNormals normals
    normals SetInput [byu GetOutput]
vtkImplicitTextureCoords tex1
    tex1 SetInput [normals GetOutput]
    tex1 SetRFunction planes
#    tex1 FlipTextureOn
vtkDataSetMapper byuMapper
    byuMapper SetInput [tex1 GetOutput]
vtkActor byuActor
    byuActor SetMapper byuMapper
    byuActor SetTexture texture
    eval [byuActor GetProperty] SetColor $cold_grey

vtkBYUReader byu2
    byu2 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu2 SetPartNumber 2
vtkPolyDataNormals normals2
    normals2 SetInput [byu2 GetOutput]
vtkImplicitTextureCoords tex2
    tex2 SetInput [normals2 GetOutput]
    tex2 SetRFunction planes
#    tex2 FlipTextureOn
vtkDataSetMapper byuMapper2
    byuMapper2 SetInput [tex2 GetOutput]
vtkActor byuActor2
    byuActor2 SetMapper byuMapper2
    byuActor2 SetTexture texture
    eval [byuActor2 GetProperty] SetColor $peacock

vtkBYUReader byu3
    byu3 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu3 SetPartNumber 3

vtkTriangleFilter triangle3
  triangle3 SetInput [byu3 GetOutput]

vtkPolyDataNormals normals3
    normals3 SetInput [triangle3 GetOutput]
vtkImplicitTextureCoords tex3
    tex3 SetInput [normals3 GetOutput]
    tex3 SetRFunction planes
#    tex3 FlipTextureOn
vtkDataSetMapper byuMapper3
    byuMapper3 SetInput [tex3 GetOutput]
vtkActor byuActor3
    byuActor3 SetMapper byuMapper3
    byuActor3 SetTexture texture
    eval [byuActor3 GetProperty] SetColor $raw_sienna

vtkBYUReader byu4
    byu4 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu4 SetPartNumber 4
vtkPolyDataNormals normals4
    normals4 SetInput [byu4 GetOutput]
vtkImplicitTextureCoords tex4
    tex4 SetInput [normals4 GetOutput]
    tex4 SetRFunction planes
#    tex4 FlipTextureOn
vtkDataSetMapper byuMapper4
    byuMapper4 SetInput [tex4 GetOutput]
vtkActor byuActor4
    byuActor4 SetMapper byuMapper4
    byuActor4 SetTexture texture
    eval [byuActor4 GetProperty] SetColor $banana

vtkBYUReader byu5
    byu5 SetGeometryFileName "$VTK_DATA_ROOT/Data/motor.g"
    byu5 SetPartNumber 5
vtkPolyDataNormals normals5
    normals5 SetInput [byu5 GetOutput]
vtkImplicitTextureCoords tex5
    tex5 SetInput [normals5 GetOutput]
    tex5 SetRFunction planes
#    tex5 FlipTextureOn
vtkDataSetMapper byuMapper5
    byuMapper5 SetInput [tex5 GetOutput]
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

