package require vtk
package require vtkinteraction
package require vtktesting

# Create ren1dering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# ingest data file
vtkPolyDataReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/vtk.vtk"

# create implicit model of vtk
vtkImplicitModeller imp
  imp SetInput [reader GetOutput]
  imp SetSampleDimensions 50 50 40
  imp SetMaximumDistance 0.25
  imp SetAdjustDistance 0.5

# create swept surface
vtkTransformCollection transforms
vtkTransform t1
  t1 Identity
vtkTransform t2
  t2 Translate 0 0 2.5
  t2 RotateZ 90.0
transforms AddItem t1
transforms AddItem t2

vtkSweptSurface sweptSurfaceFilter
  sweptSurfaceFilter SetInput [imp GetOutput]
  sweptSurfaceFilter SetTransforms transforms
  sweptSurfaceFilter SetSampleDimensions 100 70 40
  sweptSurfaceFilter SetNumberOfInterpolationSteps 20

vtkMarchingContourFilter iso
  iso SetInput [sweptSurfaceFilter GetOutput]
  iso SetValue 0 0.33

vtkPolyDataMapper sweptSurfaceMapper
  sweptSurfaceMapper SetInput [iso GetOutput]
  sweptSurfaceMapper ScalarVisibilityOff

vtkActor sweptSurface
  sweptSurface SetMapper sweptSurfaceMapper
  [sweptSurface GetProperty] SetColor 0.2510 0.8784 0.8157

ren1 AddActor sweptSurface
ren1 SetBackground 1 1 1

renWin SetSize 300 300

iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .












