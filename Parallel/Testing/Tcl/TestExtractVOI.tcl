package require vtk
package require vtkinteraction

# to mark the origin
vtkSphereSource sphere
  sphere SetRadius 2.0
vtkPolyDataMapper sphereMapper
  sphereMapper SetInput [sphere GetOutput]
  sphereMapper ImmediateModeRenderingOn
vtkActor sphereActor
  sphereActor SetMapper sphereMapper


vtkRTAnalyticSource rt
rt SetWholeExtent -50 50 -50 50 0 0

vtkExtractVOI voi
  voi SetInput [rt GetOutput]
  voi SetVOI -11 39 5 45 0 0
  voi SetSampleRate 5 5 1

# Get rid ambiguous triagulation issues.
vtkDataSetSurfaceFilter surf
  surf SetInput [voi GetOutput]
vtkTriangleFilter tris
  tris SetInput [surf GetOutput]

vtkPolyDataMapper mapper
  mapper SetInput [tris GetOutput]
  mapper ImmediateModeRenderingOn
  mapper SetScalarRange 130 280
vtkActor actor
  actor SetMapper mapper


vtkRenderer ren
ren AddActor actor
ren AddActor sphereActor
set camera [ren GetActiveCamera]

#$camera SetPosition 68.1939 -23.4323 12.6465
#$camera SetViewUp 0.46563 0.882375 0.0678508  
#$camera SetFocalPoint 3.65707 11.4552 1.83509 
#$camera SetClippingRange 59.2626 101.825 

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

wm withdraw .

