catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# splat points to generate surface
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read cyberware file
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA/fran_cut.vtk"
vtkPolyDataNormals normals
    normals SetInput [cyber GetOutput]
vtkMaskPoints mask
    mask SetInput [normals GetOutput]
    mask SetOnRatio 50
#    mask RandomModeOn
vtkGaussianSplatter splatter
    splatter SetInput [mask GetOutput]
    splatter SetSampleDimensions 100 100 100
    splatter SetEccentricity 2.5
    splatter NormalWarpingOn
    splatter SetScaleFactor 1.0
    splatter SetRadius 0.025
vtkContourFilter contour
    contour SetInput [splatter GetOutput]
    contour SetValue 0 0.25
vtkPolyDataMapper splatMapper
    splatMapper SetInput [contour GetOutput]
    splatMapper ScalarVisibilityOff
vtkActor splatActor
    splatActor SetMapper splatMapper
    eval [splatActor GetProperty] SetColor 1.0 0.49 0.25

vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [cyber GetOutput]
    cyberMapper ScalarVisibilityOff
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
    [cyberActor GetProperty] SetRepresentationToWireframe
    eval [cyberActor GetProperty] SetColor 0.2510 0.8784 0.8157

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 AddActor splatActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 SetBackground 1 1 1
vtkCamera camera
    camera SetClippingRange 0.0332682 1.66341
    camera SetFocalPoint 0.0511519 -0.127555 -0.0554379
    camera SetPosition 0.516567 -0.124763 -0.349538
    camera SetViewAngle 18.1279
    camera SetViewUp -0.013125 0.99985 -0.0112779
ren1 SetActiveCamera camera

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
#renWin SetFileName "splatFace.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

