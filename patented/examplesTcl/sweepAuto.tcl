catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# ingest data file
vtkUGFacetReader reader
  reader SetFileName ../../../vtkdata/bolt.fac
  reader MergingOff

# create implicit model of vtk
vtkImplicitModeller imp
  imp SetInput [reader GetOutput]
  imp SetSampleDimensions 25 25 50
  imp SetMaximumDistance 0.33
  imp SetAdjustDistance 0.75

# create swept surface
vtkMath math

vtkTransformCollection transforms
vtkTransform t1
  t1 Identity
transforms AddItem t1

for {set i 2} {$i <= 10} {incr i} {
  vtkTransform t$i
  t$i Translate [math Random -4 4] [math Random -4 4] [math Random -4 4]
  t$i RotateZ [math Random -180 180]
  t$i RotateX [math Random -180 180]
  t$i RotateY [math Random -180 180]
  transforms AddItem t$i
}

vtkSweptSurface sweptSurfaceFilter
  sweptSurfaceFilter SetInput [imp GetOutput]
  sweptSurfaceFilter SetTransforms transforms
  sweptSurfaceFilter SetSampleDimensions 100 100 100
  sweptSurfaceFilter SetNumberOfInterpolationSteps 0
  sweptSurfaceFilter SetMaximumNumberOfInterpolationSteps 80
  sweptSurfaceFilter CappingOff

vtkMarchingContourFilter iso
  iso SetInput [sweptSurfaceFilter GetOutput]
  iso SetValue 0 0.075

vtkPolyDataMapper sweptSurfaceMapper
  sweptSurfaceMapper SetInput [iso GetOutput]
  sweptSurfaceMapper ScalarVisibilityOff

vtkActor sweptSurface
  sweptSurface SetMapper sweptSurfaceMapper
  [sweptSurface GetProperty] SetDiffuseColor 0.2510 0.8784 0.8157

ren1 AddActor sweptSurface
ren1 SetBackground 1 1 1

renWin SetSize 500 500

iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

#renWin SetFileName "sweptAuto.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



