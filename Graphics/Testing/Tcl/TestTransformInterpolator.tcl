package require vtk
package require vtkinteraction
package require vtktesting

# create two boxes and interpolate between them
#
vtkPoints pts
  pts InsertNextPoint -1 -1 -1
  pts InsertNextPoint  1 -1 -1
  pts InsertNextPoint  1  1 -1
  pts InsertNextPoint -1  1 -1
  pts InsertNextPoint -1 -1  1
  pts InsertNextPoint  1 -1  1
  pts InsertNextPoint  1  1  1
  pts InsertNextPoint -1  1  1
vtkCellArray faces
  faces InsertNextCell 4 
  faces InsertCellPoint 0 
  faces InsertCellPoint 1 
  faces InsertCellPoint 2 
  faces InsertCellPoint 3
  faces InsertNextCell 4 
  faces InsertCellPoint 4 
  faces InsertCellPoint 5 
  faces InsertCellPoint 6 
  faces InsertCellPoint 7
  faces InsertNextCell 4 
  faces InsertCellPoint 0 
  faces InsertCellPoint 1 
  faces InsertCellPoint 5 
  faces InsertCellPoint 4
  faces InsertNextCell 4 
  faces InsertCellPoint 1 
  faces InsertCellPoint 2 
  faces InsertCellPoint 6 
  faces InsertCellPoint 5
  faces InsertNextCell 4 
  faces InsertCellPoint 2 
  faces InsertCellPoint 3 
  faces InsertCellPoint 7 
  faces InsertCellPoint 6
  faces InsertNextCell 4 
  faces InsertCellPoint 3 
  faces InsertCellPoint 0 
  faces InsertCellPoint 4 
  faces InsertCellPoint 7
vtkUnsignedCharArray faceColors
  faceColors SetNumberOfComponents 3
  faceColors SetNumberOfTuples 3
  faceColors InsertComponent 0 0 255
  faceColors InsertComponent 0 1 0
  faceColors InsertComponent 0 2 0
  faceColors InsertComponent 1 0 0
  faceColors InsertComponent 1 1 255
  faceColors InsertComponent 1 2 0
  faceColors InsertComponent 2 0 255
  faceColors InsertComponent 2 1 255
  faceColors InsertComponent 2 2 0
  faceColors InsertComponent 3 0 0
  faceColors InsertComponent 3 1 0
  faceColors InsertComponent 3 2 255
  faceColors InsertComponent 4 0 255
  faceColors InsertComponent 4 1 0
  faceColors InsertComponent 4 2 255
  faceColors InsertComponent 5 0 0
  faceColors InsertComponent 5 1 255
  faceColors InsertComponent 5 2 255

vtkPolyData cube
  cube SetPoints pts
  cube SetPolys faces
  [cube GetCellData] SetScalars faceColors

vtkPolyDataMapper cube1Mapper
    cube1Mapper SetInput cube
    cube1Mapper SetScalarRange 0 5
vtkActor cube1
    cube1 SetMapper cube1Mapper
    [cube1 GetProperty] SetAmbient 0.4
    cube1 SetPosition 1 2 3
    cube1 SetScale 4 2 1
    cube1 RotateX 15

vtkPolyDataMapper cube2Mapper
    cube2Mapper SetInput cube
    cube2Mapper SetScalarRange 0 5
vtkActor cube2
    cube2 SetMapper cube2Mapper
    cube2 SetPosition 5 10 15
    cube2 SetScale 1 2 4
    cube2 RotateX 22.5
    cube2 RotateY 15
    cube2 RotateZ 85
    [cube2 GetProperty] SetAmbient 0.4

vtkPolyDataMapper cube3Mapper
    cube3Mapper SetInput cube
    cube3Mapper SetScalarRange 0 5
vtkActor cube3
    cube3 SetMapper cube3Mapper
    cube3 SetPosition 5 -10 15
    cube3 SetScale 2 4 1
    cube3 RotateX 13
    cube3 RotateY 72
    cube3 RotateZ -15
    [cube3 GetProperty] SetAmbient 0.4

vtkPolyDataMapper cube4Mapper
    cube4Mapper SetInput cube
    cube4Mapper SetScalarRange 0 5
vtkActor cube4
    cube4 SetMapper cube4Mapper
    cube4 SetPosition 10 -5 5
    cube4 SetScale 2 .5 1
    cube4 RotateX 66
    cube4 RotateY 19
    cube4 RotateZ 24
    [cube4 GetProperty] SetAmbient 0.4

# Interpolate the transformation
vtkPolyDataMapper cubeMapper
    cubeMapper SetInput cube
    cubeMapper SetScalarRange 0 6
vtkActor cubeActor
    cubeActor SetMapper cubeMapper
    [cubeActor GetProperty] SetAmbient 0.4

# Interpolate some transformations, test along the way
vtkTransformInterpolator interpolator
#interpolator SetInterpolationTypeToLinear
interpolator SetInterpolationTypeToSpline
interpolator AddTransform 0.0 cube1
interpolator AddTransform 8.0 cube2
interpolator AddTransform 18.2 cube3
interpolator AddTransform 24.4 cube4
interpolator Initialize
#puts [interpolator GetNumberOfTransforms]
interpolator AddTransform 0.0 cube1
interpolator AddTransform 8.0 cube2
interpolator AddTransform 18.2 cube3
interpolator AddTransform 24.4 cube4
#puts [interpolator GetNumberOfTransforms]

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cube1
ren1 AddActor cube2
ren1 AddActor cube3
ren1 AddActor cube4
ren1 AddActor cubeActor

ren1 SetBackground 0 0 0
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
vtkCamera camera
camera SetClippingRange 31.2977 81.697
camera SetFocalPoint 3.0991 -2.00445 9.78648
camera SetPosition -44.8481 -25.871 10.0645
camera SetViewAngle 30
camera SetViewUp -0.0356378 0.0599728 -0.997564

vtkLight light
eval light SetPosition [camera GetPosition]
eval light SetFocalPoint [camera GetFocalPoint]

ren1 SetActiveCamera camera
ren1 AddLight light

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

vtkTransform xform
proc animate {} {
    set numSteps 250
    set min [interpolator GetMinimumT]
    set max [interpolator GetMaximumT]
    for {set i 0} {$i <= $numSteps} {incr i} {
        set t [expr double($i) * ($max - $min) / double($numSteps)]
        interpolator InterpolateTransform $t xform
        cubeActor SetUserMatrix [xform GetMatrix]
        renWin Render
    }
}
interpolator InterpolateTransform 13.2 xform
cubeActor SetUserMatrix [xform GetMatrix]
renWin Render
#animate

    
