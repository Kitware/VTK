
package require vtk

vtkRandomGraphSource src

vtkGraphLayoutView view
view AddRepresentationFromInputConnection [src GetOutputPort]

vtkRenderWindow window
view SetupRenderWindow window
[window GetInteractor] Start

wm withdraw .

