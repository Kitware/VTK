package require vtktcl_interactor

vtkSphereSource ss
   ss SetPhiResolution 10
   ss SetThetaResolution 20

vtkPLYWriter w
   w SetInput [ss GetOutput]
   w SetFileName plyWriter.ply
   w SetFileTypeToASCII
   w SetDataByteOrderToLittleEndian
   w Write

vtkPLYReader r
   r SetFileName plyWriter.ply
   r Update
file delete -force plyWriter.ply

vtkPolyDataMapper plyMapper
   plyMapper SetInput [r GetOutput]
vtkActor plyActor
   plyActor SetMapper plyMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor plyActor

renWin SetSize 250 250
iren Initialize
renWin Render
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
