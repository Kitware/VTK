package require vtk
package require vtkinteraction
package require vtktesting

# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkOBJReader cowReader
  cowReader SetFileName "$VTK_DATA_ROOT/Data/Viewpoint/cow.obj"

vtkPlane plane
  plane SetNormal 1 0 0

vtkClipPolyData cowClipper
  cowClipper SetInput [cowReader GetOutput]
  cowClipper SetClipFunction plane

vtkPolyDataNormals cellNormals
  cellNormals SetInput [cowClipper GetOutput]
  cellNormals ComputePointNormalsOn
  cellNormals ComputeCellNormalsOn

vtkTransform reflect
  reflect Scale -1 1 1

vtkTransformPolyDataFilter cowReflect
  cowReflect SetTransform reflect
  cowReflect SetInput [cellNormals GetOutput]

vtkReverseSense cowReverse
  cowReverse SetInput [cowReflect GetOutput]
  cowReverse ReverseNormalsOn
  cowReverse ReverseCellsOff

vtkPolyDataMapper reflectedMapper
  reflectedMapper SetInput [cowReverse GetOutput]

vtkActor reflected
  reflected SetMapper reflectedMapper
eval [reflected GetProperty] SetDiffuseColor $flesh
[reflected GetProperty] SetDiffuse .8
[reflected GetProperty] SetSpecular .5
[reflected GetProperty] SetSpecularPower 30
[reflected GetProperty] FrontfaceCullingOn

ren1 AddActor reflected

vtkPolyDataMapper cowMapper
    cowMapper SetInput [cowClipper GetOutput]

vtkActor cow
    cow SetMapper cowMapper

ren1 AddActor cow

ren1 SetBackground .1 .2 .4
renWin SetSize 320 240
[ren1 GetActiveCamera] SetViewUp 0 1 0
[ren1 GetActiveCamera] Azimuth 180
[ren1 GetActiveCamera] Dolly 1.75
ren1 ResetCameraClippingRange

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .


