package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPolyDataReader pdreader
    pdreader SetFileName "$VTK_DATA_ROOT/Data/brainImageSmooth.vtk"
vtkTriangleFilter tf
    tf SetInput [pdreader GetOutput]

vtkDecimate deci; 
    deci SetInput [tf GetOutput]
    deci SetTargetReduction 0.9
    deci SetAspectRatio 20
    deci SetInitialError 0.0002
    deci SetErrorIncrement 0.0005
    deci SetMaximumIterations 6
    deci SetInitialFeatureAngle 45
vtkPolyDataNormals normals
    normals SetInput [deci GetOutput]
vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [normals GetOutput]
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
    eval [cyberActor GetProperty] SetColor 1.0 0.49 0.25
    eval [cyberActor GetProperty] SetRepresentationToWireframe

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


