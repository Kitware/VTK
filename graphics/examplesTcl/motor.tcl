catch {load vtktcl}
# this is a tcl version of motor visualization
# get the interactor ui
source vtkInt.tcl
source colors.tcl

# First create the render master
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create cutting planes
vtkPlanes planes
vtkFloatPoints points
vtkFloatNormals norms

points InsertPoint 0 0.0 0.0 0.0
norms InsertNormal 0 0.0 0.0 1.0;    
points InsertPoint 1 0.0 0.0 0.0
norms InsertNormal 1 -1.0 0.0 0.0;    

planes SetPoints points
planes SetNormals norms

# texture
vtkStructuredPointsReader texReader
    texReader SetFileName "../../../data/texThres.vtk"
vtkTexture texture
    texture SetInput [texReader GetOutput]
    texture InterpolateOff
    texture RepeatOff

# read motor parts...each part colored separately
#
vtkBYUReader byu
    byu SetGeometryFileName "../../../data/motor.g"
    byu SetPartNumber 1
vtkPolyNormals normals
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
    byu2 SetGeometryFileName "../../../data/motor.g"
    byu2 SetPartNumber 2
vtkPolyNormals normals2
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
    byu3 SetGeometryFileName "../../../data/motor.g"
    byu3 SetPartNumber 3
vtkPolyNormals normals3
    normals3 SetInput [byu3 GetOutput]
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
    byu4 SetGeometryFileName "../../../data/motor.g"
    byu4 SetPartNumber 4
vtkPolyNormals normals4
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
    byu5 SetGeometryFileName "../../../data/motor.g"
    byu5 SetPartNumber 5
vtkPolyNormals normals5
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
$ren1 AddActors byuActor
$ren1 AddActors byuActor2
$ren1 AddActors byuActor3
$ren1 AddActors byuActor4
$ren1 AddActors byuActor5
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

vtkCamera camera
    camera SetFocalPoint 0.0286334 0.0362996 0.0379685
    camera SetPosition 1.37067 1.08629 -1.30349
    camera CalcViewPlaneNormal
    camera SetViewAngle 17.673
    camera SetViewUp  -0.376306 -0.5085 -0.774482

$ren1 SetActiveCamera camera

# render the image
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
$renWin SetFileName "motor.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

