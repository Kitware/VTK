package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a soccer ball
#
vtkPoints points
# first point repeated because polygons were 1-offset
points InsertNextPoint     0.348012            0      0.93749
points InsertNextPoint     0.348012            0      0.93749
points InsertNextPoint     0.107542     0.330979      0.93749
points InsertNextPoint    -0.281548     0.204556      0.93749
points InsertNextPoint    -0.281548    -0.204556      0.93749
points InsertNextPoint     0.107542    -0.330979      0.93749
points InsertNextPoint     0.694318            0     0.719669
points InsertNextPoint     0.799191    -0.327801     0.502204
points InsertNextPoint     0.965027     -0.20654     0.154057
points InsertNextPoint     0.965027      0.20654     0.154057
points InsertNextPoint     0.799191     0.327801     0.502204
points InsertNextPoint     0.214556     0.660335     0.719669
points InsertNextPoint     0.558721      0.65878     0.502204
points InsertNextPoint     0.494641     0.853971     0.154057
points InsertNextPoint     0.101778     0.981619     0.154057
points InsertNextPoint   -0.0647933     0.861372     0.502204
points InsertNextPoint    -0.561715      0.40811     0.719669
points InsertNextPoint    -0.453883     0.734949     0.502204
points InsertNextPoint    -0.659322     0.734323     0.154057
points InsertNextPoint    -0.902124     0.400134     0.154057
points InsertNextPoint    -0.839236     0.204556     0.502204
points InsertNextPoint    -0.561715     -0.40811     0.719669
points InsertNextPoint    -0.839236    -0.204556     0.502204
points InsertNextPoint    -0.902124    -0.400134     0.154057
points InsertNextPoint    -0.659322    -0.734323     0.154057
points InsertNextPoint    -0.453883    -0.734949     0.502204
points InsertNextPoint     0.214556    -0.660335     0.719669
points InsertNextPoint   -0.0647933    -0.861372     0.502204
points InsertNextPoint     0.101778    -0.981619     0.154057
points InsertNextPoint     0.494641    -0.853971     0.154057
points InsertNextPoint     0.558721     -0.65878     0.502204
points InsertNextPoint     0.902124     0.400134    -0.154057
points InsertNextPoint     0.839236     0.204556    -0.502204
points InsertNextPoint     0.561715      0.40811    -0.719669
points InsertNextPoint     0.453883     0.734949    -0.502204
points InsertNextPoint     0.659322     0.734323    -0.154057
points InsertNextPoint    -0.101778     0.981619    -0.154057
points InsertNextPoint    0.0647933     0.861372    -0.502204
points InsertNextPoint    -0.214556     0.660335    -0.719669
points InsertNextPoint    -0.558721      0.65878    -0.502204
points InsertNextPoint    -0.494641     0.853971    -0.154057
points InsertNextPoint    -0.965027      0.20654    -0.154057
points InsertNextPoint    -0.799191     0.327801    -0.502204
points InsertNextPoint    -0.694318            0    -0.719669
points InsertNextPoint    -0.799191    -0.327801    -0.502204
points InsertNextPoint    -0.965027     -0.20654    -0.154057
points InsertNextPoint    -0.494641    -0.853971    -0.154057
points InsertNextPoint    -0.558721     -0.65878    -0.502204
points InsertNextPoint    -0.214556    -0.660335    -0.719669
points InsertNextPoint    0.0647933    -0.861372    -0.502204
points InsertNextPoint    -0.101778    -0.981619    -0.154057
points InsertNextPoint     0.659322    -0.734323    -0.154057
points InsertNextPoint     0.453883    -0.734949    -0.502204
points InsertNextPoint     0.561715     -0.40811    -0.719669
points InsertNextPoint     0.839236    -0.204556    -0.502204
points InsertNextPoint     0.902124    -0.400134    -0.154057
points InsertNextPoint     0.281548    -0.204556     -0.93749
points InsertNextPoint    -0.107542    -0.330979     -0.93749
points InsertNextPoint    -0.348012            0     -0.93749
points InsertNextPoint    -0.107542     0.330979     -0.93749
points InsertNextPoint     0.281548     0.204556     -0.93749

vtkCellArray faces
faces InsertNextCell 5
faces InsertCellPoint    5
faces InsertCellPoint    4
faces InsertCellPoint    3
faces InsertCellPoint    2
faces InsertCellPoint    1
faces InsertNextCell 5
faces InsertCellPoint   10
faces InsertCellPoint    9
faces InsertCellPoint    8
faces InsertCellPoint    7
faces InsertCellPoint    6
faces InsertNextCell 5
faces InsertCellPoint   15
faces InsertCellPoint   14
faces InsertCellPoint   13
faces InsertCellPoint   12
faces InsertCellPoint   11
faces InsertNextCell 5
faces InsertCellPoint   20
faces InsertCellPoint   19
faces InsertCellPoint   18
faces InsertCellPoint   17
faces InsertCellPoint   16
faces InsertNextCell 5
faces InsertCellPoint   25
faces InsertCellPoint   24
faces InsertCellPoint   23
faces InsertCellPoint   22
faces InsertCellPoint   21
faces InsertNextCell 5
faces InsertCellPoint   30
faces InsertCellPoint   29
faces InsertCellPoint   28
faces InsertCellPoint   27
faces InsertCellPoint   26
faces InsertNextCell 5
faces InsertCellPoint   35
faces InsertCellPoint   34
faces InsertCellPoint   33
faces InsertCellPoint   32
faces InsertCellPoint   31
faces InsertNextCell 5
faces InsertCellPoint   40
faces InsertCellPoint   39
faces InsertCellPoint   38
faces InsertCellPoint   37
faces InsertCellPoint   36
faces InsertNextCell 5
faces InsertCellPoint   45
faces InsertCellPoint   44
faces InsertCellPoint   43
faces InsertCellPoint   42
faces InsertCellPoint   41
faces InsertNextCell 5
faces InsertCellPoint   50
faces InsertCellPoint   49
faces InsertCellPoint   48
faces InsertCellPoint   47
faces InsertCellPoint   46
faces InsertNextCell 5
faces InsertCellPoint   55
faces InsertCellPoint   54
faces InsertCellPoint   53
faces InsertCellPoint   52
faces InsertCellPoint   51
faces InsertNextCell 5
faces InsertCellPoint   60
faces InsertCellPoint   59
faces InsertCellPoint   58
faces InsertCellPoint   57
faces InsertCellPoint   56
faces InsertNextCell 6
faces InsertCellPoint    2
faces InsertCellPoint   11
faces InsertCellPoint   12
faces InsertCellPoint   10
faces InsertCellPoint    6
faces InsertCellPoint    1
faces InsertNextCell 6
faces InsertCellPoint    3
faces InsertCellPoint   16
faces InsertCellPoint   17
faces InsertCellPoint   15
faces InsertCellPoint   11
faces InsertCellPoint   2
faces InsertNextCell 6
faces InsertCellPoint    4
faces InsertCellPoint   21
faces InsertCellPoint   22
faces InsertCellPoint   20
faces InsertCellPoint   16
faces InsertCellPoint        3
faces InsertNextCell 6
faces InsertCellPoint    5
faces InsertCellPoint   26
faces InsertCellPoint   27
faces InsertCellPoint   25
faces InsertCellPoint   21
faces InsertCellPoint        4
faces InsertNextCell 6
faces InsertCellPoint    1
faces InsertCellPoint    6
faces InsertCellPoint    7
faces InsertCellPoint   30
faces InsertCellPoint   26
faces InsertCellPoint        5
faces InsertNextCell 6
faces InsertCellPoint   12
faces InsertCellPoint   13
faces InsertCellPoint   35
faces InsertCellPoint   31
faces InsertCellPoint    9
faces InsertCellPoint        10
faces InsertNextCell 6
faces InsertCellPoint   17
faces InsertCellPoint   18
faces InsertCellPoint   40
faces InsertCellPoint   36
faces InsertCellPoint   14
faces InsertCellPoint       15
faces InsertNextCell 6
faces InsertCellPoint   22
faces InsertCellPoint   23
faces InsertCellPoint   45
faces InsertCellPoint   41
faces InsertCellPoint   19
faces InsertCellPoint       20
faces InsertNextCell 6
faces InsertCellPoint   27
faces InsertCellPoint   28
faces InsertCellPoint   50
faces InsertCellPoint   46
faces InsertCellPoint   24
faces InsertCellPoint       25
faces InsertNextCell 6
faces InsertCellPoint    7
faces InsertCellPoint    8
faces InsertCellPoint   55
faces InsertCellPoint   51
faces InsertCellPoint   29
faces InsertCellPoint       30
faces InsertNextCell 6
faces InsertCellPoint    9
faces InsertCellPoint   31
faces InsertCellPoint   32
faces InsertCellPoint   54
faces InsertCellPoint   55
faces InsertCellPoint        8
faces InsertNextCell 6
faces InsertCellPoint   14
faces InsertCellPoint   36
faces InsertCellPoint   37
faces InsertCellPoint   34
faces InsertCellPoint   35
faces InsertCellPoint       13
faces InsertNextCell 6
faces InsertCellPoint   19
faces InsertCellPoint   41
faces InsertCellPoint   42
faces InsertCellPoint   39
faces InsertCellPoint   40
faces InsertCellPoint       18
faces InsertNextCell 6
faces InsertCellPoint   24
faces InsertCellPoint   46
faces InsertCellPoint   47
faces InsertCellPoint   44
faces InsertCellPoint   45
faces InsertCellPoint       23
faces InsertNextCell 6
faces InsertCellPoint   29
faces InsertCellPoint   51
faces InsertCellPoint   52
faces InsertCellPoint   49
faces InsertCellPoint   50
faces InsertCellPoint       28
faces InsertNextCell 6
faces InsertCellPoint   32
faces InsertCellPoint   33
faces InsertCellPoint   60
faces InsertCellPoint   56
faces InsertCellPoint   53
faces InsertCellPoint       54
faces InsertNextCell 6
faces InsertCellPoint   37
faces InsertCellPoint   38
faces InsertCellPoint   59
faces InsertCellPoint   60
faces InsertCellPoint   33
faces InsertCellPoint       34
faces InsertNextCell 6
faces InsertCellPoint   42
faces InsertCellPoint   43
faces InsertCellPoint   58
faces InsertCellPoint   59
faces InsertCellPoint   38
faces InsertCellPoint       39
faces InsertNextCell 6
faces InsertCellPoint   47
faces InsertCellPoint   48
faces InsertCellPoint   57
faces InsertCellPoint   58
faces InsertCellPoint   43
faces InsertCellPoint       44
faces InsertNextCell 6
faces InsertCellPoint   52
faces InsertCellPoint   53
faces InsertCellPoint   56
faces InsertCellPoint   57
faces InsertCellPoint   48
faces InsertCellPoint       49

vtkFloatArray faceColors
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2
  faceColors InsertNextValue 2

vtkFloatArray vertexColors
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2
  vertexColors InsertNextValue 2

vtkPolyData model
  model SetPolys faces
  model SetPoints points
  [model GetCellData] SetScalars faceColors
  [model GetPointData] SetScalars vertexColors

vtkTextureMapToSphere ballTC
  ballTC SetInput model

vtkLookupTable lut
  lut SetNumberOfColors 3
  lut Build
  lut SetTableValue 0 0 0 0 0
  lut SetTableValue 1 1 .3 .3 1
  lut SetTableValue 2 .8 .8 .9 1

vtkDataSetMapper mapper
    mapper SetInputConnection [ballTC GetOutputPort]
    mapper SetScalarModeToUseCellData
    mapper SetLookupTable lut
    mapper SetScalarRange 0 2

vtkPNMReader earth
  earth SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkTexture texture
texture SetInputConnection [earth GetOutputPort]

vtkActor soccerBall
    soccerBall SetMapper mapper
  soccerBall SetTexture texture

# Add the actors to the renderer, set the background and size
#
ren1 AddActor soccerBall
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
[ren1 GetActiveCamera] SetPosition 4.19682 4.65178 6.23545 
[ren1 GetActiveCamera] SetFocalPoint 0 0 0 
[ren1 GetActiveCamera] SetViewAngle 21.4286
[ren1 GetActiveCamera] SetViewUp 0.451577 -0.833646 0.317981 

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
ren1 ResetCameraClippingRange

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


