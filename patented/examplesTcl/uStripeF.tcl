catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version of old strip unstructured data
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a cyberware source
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA/fran_cut.vtk"
vtkDecimate deci
    deci SetInput [cyber GetOutput]
    deci SetTargetReduction 0.90
    deci SetInitialError 0.0002
    deci SetErrorIncrement 0.0004
    deci SetMaximumIterations 6
    deci SetInitialFeatureAngle 45
vtkPolyDataNormals normals
    normals SetInput [deci GetOutput]
vtkMaskPolyData mask
    mask SetInput [normals GetOutput]
    mask SetOnRatio 2
vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [mask GetOutput]
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
eval [cyberActor GetProperty] SetColor 1.0 0.49 0.25

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
vtkCamera cam1
  cam1 SetFocalPoint 0.0520703 -0.128547 -0.0581083
  cam1 SetPosition 0.419653 -0.120916 -0.321626
  cam1 SetViewAngle 21.4286
  cam1 SetViewUp -0.0136986 0.999858 0.00984497
ren1 SetActiveCamera cam1
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "uStripeF.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


