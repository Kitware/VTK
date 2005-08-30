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
  faces InsertCellPoint 3 
  faces InsertCellPoint 2 
  faces InsertCellPoint 1
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

vtkTransform t1
    t1 Translate 1 2 3
    t1 RotateX 15
    t1 Scale 4 2 1
vtkTransformPolyDataFilter tpdf1
    tpdf1 SetInput cube
    tpdf1 SetTransform t1
vtkPolyDataMapper cube1Mapper
    cube1Mapper SetInputConnection [tpdf1 GetOutputPort]
vtkActor cube1
    cube1 SetMapper cube1Mapper

vtkTransform t2
    t2 Translate 5 10 15
    t2 RotateX 22.5
    t2 RotateY 15
    t2 RotateZ 85
    t2 Scale 1 2 4
vtkTransformPolyDataFilter tpdf2
    tpdf2 SetInput cube
    tpdf2 SetTransform t2
vtkPolyDataMapper cube2Mapper
    cube2Mapper SetInputConnection [tpdf2 GetOutputPort]
vtkActor cube2
    cube2 SetMapper cube2Mapper

vtkTransform t3
    t3 Translate 5 -10 15
    t3 RotateX 13
    t3 RotateY 72
    t3 RotateZ -15
    t3 Scale 2 4 1
vtkTransformPolyDataFilter tpdf3
    tpdf3 SetInput cube
    tpdf3 SetTransform t3
vtkPolyDataMapper cube3Mapper
    cube3Mapper SetInputConnection [tpdf3 GetOutputPort]
vtkActor cube3
    cube3 SetMapper cube3Mapper

vtkTransform t4
    t4 Translate 10 -5 5
    t4 RotateX 66
    t4 RotateY 19
    t4 RotateZ 24
    t4 Scale 2 .5 1
vtkTransformPolyDataFilter tpdf4
    tpdf4 SetInput cube
    tpdf4 SetTransform t4
vtkPolyDataMapper cube4Mapper
    cube4Mapper SetInputConnection [tpdf4 GetOutputPort]
vtkActor cube4
    cube4 SetMapper cube4Mapper

# Interpolate the transformation
vtkPolyDataMapper cubeMapper
    cubeMapper SetInput cube
vtkActor cubeActor
    cubeActor SetMapper cubeMapper

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
interpolator AddTransform 0.0 t1
interpolator AddTransform 8.0 t2
interpolator AddTransform 18.2 t3
interpolator AddTransform 24.4 t4
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

ren1 SetActiveCamera camera

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

    
