catch {load vtktcl}
# this is a tcl version of motor visualization
# get the interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read a vtk file
#
vtkBYUReader byu
    byu SetGeometryFileName "../../../data/fohe.g"
vtkPolyNormals normals
    normals SetInput [byu GetOutput]
vtkPolyMapper byuMapper
    byuMapper SetInput [normals GetOutput]
vtkActor byuActor
    byuActor SetMapper byuMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor byuActor
ren1 SetBackground 0.2 0.3 0.4
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "fohe.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


