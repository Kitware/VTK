package require vtktcl
package require vtktcl_interact

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkSTLReader stl
    stl SetFileName "$VTK_DATA_ROOT/Data/42400-IDGH.stl"
vtkPolyDataNormals normals
    normals SetInput [stl GetOutput]
vtkPolyDataMapper mapper
    mapper SetInput [normals GetOutput]
vtkActor actor
    actor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


