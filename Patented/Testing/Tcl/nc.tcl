package require vtk
package require vtkinteraction
package require vtktesting

# Create rendering stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# ingest data file
vtkUGFacetReader toolModel
  toolModel SetFileName $VTK_DATA_ROOT/Data/bolt.fac
  toolModel MergingOn

# create implicit model of vtk
vtkImplicitModeller toolImp
  toolImp SetInput [toolModel GetOutput]
  toolImp SetSampleDimensions 25 25 50
  toolImp SetMaximumDistance 0.33
  toolImp SetAdjustDistance 0.75

# create swept surface
vtkTransformCollection transforms
vtkTransform t1
  t1 Identity
vtkTransform t2
  t2 Translate -1 0 0
vtkTransform t3
  t3 Translate -1 0 -1
transforms AddItem t1
transforms AddItem t2
transforms AddItem t3
toolImp Update

vtkSweptSurface toolVolume
  toolVolume SetInput [toolImp GetOutput]
  toolVolume SetTransforms transforms
  toolVolume SetSampleDimensions 50 50 50
  toolVolume SetNumberOfInterpolationSteps 0

vtkImplicitVolume toolFunc
  toolFunc SetVolume [toolVolume GetOutput]

vtkPoints points
  points InsertPoint 0 -1 0 0
  points InsertPoint 1 1 0 0
  points InsertPoint 2 0 -1 0
  points InsertPoint 3 0 1 0
  points InsertPoint 4 0 0 -1
  points InsertPoint 5 0 0 1
 
vtkFloatArray normals
  normals SetNumberOfComponents 3
  normals InsertTuple3 0 -1 0 0
  normals InsertTuple3 1 1 0 0
  normals InsertTuple3 2 0 -1 0
  normals InsertTuple3 3 0 1 0
  normals InsertTuple3 4 0 0 -1
  normals InsertTuple3 5 0 0 1
 
vtkPlanes partImp
  partImp SetPoints points
  partImp SetNormals normals

vtkImplicitBoolean boolean
  boolean SetOperationTypeToDifference
  boolean AddFunction partImp
  boolean AddFunction toolFunc

vtkSampleFunction sampleBoolean
  sampleBoolean SetImplicitFunction boolean
  sampleBoolean SetModelBounds -2 2 -2 2 -2 2
  sampleBoolean SetSampleDimensions 64 64 64

vtkMarchingContourFilter iso
  iso SetInput [sampleBoolean GetOutput]
  iso SetValue 0 -.05

vtkPolyDataMapper sweptSurfaceMapper
  sweptSurfaceMapper SetInput [iso GetOutput]
  sweptSurfaceMapper ScalarVisibilityOff

vtkActor sweptSurface
  sweptSurface SetMapper sweptSurfaceMapper
  [sweptSurface GetProperty] SetColor 0.2510 0.8784 0.8157

vtkPolyDataMapper toolMapper
  toolMapper SetInput [toolModel GetOutput]
  toolMapper ScalarVisibilityOff

vtkActor tool
  tool SetMapper toolMapper

ren1 AddActor sweptSurface
ren1 AddActor tool
ren1 SetBackground 1 1 1

renWin SetSize 250 250

iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



