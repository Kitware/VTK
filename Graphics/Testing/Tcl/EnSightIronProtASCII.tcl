package require vtk
package require vtkinteraction

# create a rendering window and renderer
vtkRenderer ren1
ren1 SetBackground 0 0 0

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 300 300

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# camera parameters
set camera [ren1 GetActiveCamera]
$camera SetPosition -54.8012 109.471 231.412
$camera SetFocalPoint 33 33 33
$camera SetViewUp 0.157687 0.942832 -0.293604
$camera SetViewAngle 30
$camera SetClippingRange 124.221 363.827

vtkGenericEnSightReader reader
reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/ironProt_ascii.case"
reader Update

vtkContourFilter Contour0
Contour0 SetInputConnection [reader GetOutputPort]
Contour0 SetValue 0 200
Contour0 SetComputeScalars 1

vtkPolyDataMapper mapper
mapper SetInputConnection [Contour0 GetOutputPort]
mapper SetImmediateModeRendering 1
mapper SetScalarRange 0 1
mapper SetScalarVisibility 1

vtkActor actor
actor SetMapper mapper
[actor GetProperty] SetRepresentationToSurface
[actor GetProperty] SetInterpolationToGouraud
ren1 AddActor actor

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
