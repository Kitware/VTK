package require vtk
package require vtkinteraction

vtkConeSource cone

vtkReflectionFilter reflect
reflect SetInput [cone GetOutput]
reflect SetPlaneToXMax

vtkReflectionFilter reflect2
reflect2 SetInput [reflect GetOutput]
reflect2 SetPlaneToYMax

vtkReflectionFilter reflect3
reflect3 SetInput [reflect2 GetOutput]
reflect3 SetPlaneToZMax

vtkDataSetMapper mapper
mapper SetInput [reflect3 GetOutput]

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