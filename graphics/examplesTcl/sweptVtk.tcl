catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create ren1dering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# ingest data file
vtkPolyDataReader reader
  reader SetFileName "../../../vtkdata/vtk.vtk"

# create implicit model of vtk
vtkImplicitModeller imp
  imp SetInput [reader GetOutput]
  imp SetSampleDimensions 50 50 40
  imp SetMaximumDistance 0.25

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
  sweptSurfaceFilter SetSampleDimensions 50 50 40
  sweptSurfaceFilter SetModelBounds -4.0  6.0  -1.0  6.0  -1.0  3.5
  sweptSurfaceFilter SetMaximumNumberOfInterpolationSteps 20

vtkContourFilter iso
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

renWin SetSize 500 500

iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

#renWin SetFileName "sweptVtk.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



