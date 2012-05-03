package require vtk
package require vtkinteraction
vtkRenderer ren1
  ren1 SetBackground 0 0 0
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin SetSize 300 300
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkFacetReader facet0
  facet0 SetFileName "$VTK_DATA_ROOT/Data/clown.facet"

vtkPolyDataMapper Mapper5
  Mapper5 SetInputConnection [facet0 GetOutputPort]
  Mapper5 SetImmediateModeRendering 1
  Mapper5 UseLookupTableScalarRangeOff
  Mapper5 SetScalarVisibility 1
  Mapper5 SetScalarModeToDefault
vtkLODActor Actor5
  Actor5 SetMapper Mapper5
  [Actor5 GetProperty] SetRepresentationToSurface
  [Actor5 GetProperty] SetInterpolationToGouraud
  [Actor5 GetProperty] SetAmbient 0.15
  [Actor5 GetProperty] SetDiffuse 0.85
  [Actor5 GetProperty] SetSpecular 0.1
  [Actor5 GetProperty] SetSpecularPower 100
  [Actor5 GetProperty] SetSpecularColor 1 1 1
  [Actor5 GetProperty] SetColor 1 1 1
  Actor5 SetNumberOfCloudPoints 30000

ren1 AddActor Actor5

vtkCamera camera
    camera SetClippingRange 3 6
    camera SetFocalPoint .1 .03 -.5
    camera SetPosition 4.4 -0.5 -.5
    camera SetViewUp 0 0 -1
ren1 SetActiveCamera camera

# enable user interface interactor
#iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
