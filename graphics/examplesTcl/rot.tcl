catch {load vtktcl}
# this is a tcl version of the Mace example
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkAxes testsource
vtkAxes originsource
vtkActor test
vtkActor origin
vtkPolyDataMapper originmapper
vtkPolyDataMapper testmapper

originsource SetScaleFactor 4000.0
originmapper SetInput [originsource GetOutput]
origin SetMapper originmapper
[origin GetProperty] SetAmbient 1.0
[origin GetProperty] SetDiffuse 0.0
ren1 AddActor origin

testsource SetScaleFactor 2000.0
testmapper SetInput [testsource GetOutput]
test SetMapper testmapper
[test GetProperty] SetAmbient 1.0
[test GetProperty] SetDiffuse 0.0
ren1 AddActor test

test SetPosition 0.0 1500.0 0.0
renWin Render

# do test rotations and renderings:
test RotateX 15.0
renWin Render

test RotateZ 30.0
renWin Render

test RotateY 45.0
renWin Render

#renWin SetFileName rot.tcl.ppm
#renWin SaveImageAsPPM

