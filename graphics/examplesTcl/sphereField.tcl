catch {load vtktcl}

# This example demonstrates the reading of field data associated with
# dataset point data and cell data.

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create pipeline
#
vtkPolyDataReader reader
    reader SetFileName "../../../vtkdata/sphereField.vtk"
vtkPolyDataMapper mapper
    mapper SetInput [reader GetOutput]
    eval mapper SetScalarRange [[reader GetOutput] GetScalarRange]
vtkActor actor
    actor SetMapper mapper

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

#renWin SetFileName "sphereField.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
