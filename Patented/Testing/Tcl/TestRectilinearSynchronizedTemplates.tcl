package require vtk
package require vtkinteraction

# create pipeline - rectilinear grid
#
vtkRectilinearGridReader rgridReader
rgridReader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
rgridReader Update

vtkRectilinearSynchronizedTemplates contour
contour SetInput [rgridReader GetOutput]
contour SetValue 0 1
contour ComputeScalarsOff
contour ComputeNormalsOn
contour ComputeGradientsOn

vtkPolyDataMapper cMapper
cMapper SetInput [contour GetOutput]

vtkActor cActor
cActor SetMapper cMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 200 200

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

ren1 AddActor cActor

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


