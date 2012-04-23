package require vtk
package require vtkinteraction

vtkConeSource cone

vtkReflectionFilter reflect
reflect SetInputConnection [cone GetOutputPort]
reflect SetPlaneToXMax

vtkReflectionFilter reflect2
reflect2 SetInputConnection [reflect GetOutputPort]
reflect2 SetPlaneToYMax

vtkReflectionFilter reflect3
reflect3 SetInputConnection [reflect2 GetOutputPort]
reflect3 SetPlaneToZMax

vtkDataSetMapper mapper
mapper SetInputConnection [reflect3 GetOutputPort]

vtkActor actor
actor SetMapper mapper

vtkRenderer ren1
ren1 AddActor actor

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 200 200

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

renWin Render

wm withdraw .