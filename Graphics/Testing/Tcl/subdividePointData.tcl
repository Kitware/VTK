package require vtk
package require vtkinteraction

#
# Test butterfly subdivision of point data
#




vtkSphereSource sphere
    sphere SetPhiResolution 11
    sphere SetThetaResolution 11

vtkElevationFilter colorIt
  colorIt SetInput [sphere GetOutput]
  colorIt SetLowPoint 0 0 -.5
  colorIt SetHighPoint 0 0 .5

vtkButterflySubdivisionFilter butterfly
  butterfly SetInput [colorIt GetPolyDataOutput]
  butterfly SetNumberOfSubdivisions 3

vtkLookupTable lut
  lut SetNumberOfColors 256
  lut Build

vtkPolyDataMapper mapper
  mapper SetInput [butterfly GetOutput] 
  mapper SetLookupTable lut

vtkActor actor
  actor SetMapper mapper

vtkLinearSubdivisionFilter linear
  linear SetInput [colorIt GetPolyDataOutput]
  linear SetNumberOfSubdivisions 3

vtkPolyDataMapper mapper2
  mapper2 SetInput [linear GetOutput] 
  mapper2 SetLookupTable lut

vtkActor actor2
  actor2 SetMapper mapper2

vtkPolyDataMapper mapper3
  mapper3 SetInput [colorIt GetOutput] 
  mapper3 SetLookupTable lut

vtkActor actor3
  actor3 SetMapper mapper3

vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3

vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1

ren2 AddActor actor2
ren2 SetBackground 1 1 1

ren3 AddActor actor3
ren3 SetBackground 1 1 1

renWin SetSize 600 200
vtkCamera aCamera
  aCamera Azimuth 70
vtkLight aLight
eval aLight SetPosition [aCamera GetPosition]
eval aLight SetFocalPoint [aCamera GetFocalPoint]

ren1 SetActiveCamera aCamera
ren1 AddLight aLight
ren1 ResetCamera
aCamera Dolly 1.4
ren1 ResetCameraClippingRange

ren2 SetActiveCamera aCamera
ren2 AddLight aLight

ren3 SetActiveCamera aCamera
ren3 AddLight aLight

ren3 SetViewport 0 0 .33 1
ren2 SetViewport .33 0 .67 1
ren1 SetViewport .67 0 1 1

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

proc flat {} {
    [actor GetProperty] SetInterpolationToFlat
    [actor2 GetProperty] SetInterpolationToFlat
    [actor3 GetProperty] SetInterpolationToFlat
    renWin Render
}

proc smooth {} {
    [actor GetProperty] SetInterpolationToGouraud
    [actor2 GetProperty] SetInterpolationToGouraud
    [actor3 GetProperty] SetInterpolationToGouraud
    renWin Render
}


wm withdraw .
