catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# create pipeline
# reader reads slices
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA/fullHead/headsq"
    v16 SetDataSpacing 0.8 0.8 1.5
    v16 SetImageRange 30 50
    v16 SetDataMask 0x7fff

# create points on edges
vtkEdgePoints edgePoints
  edgePoints SetInput [v16 GetOutput]
  edgePoints SetValue 1150

#
vtkDataSetMapper mapper
  mapper SetInput [edgePoints GetOutput]
  mapper ImmediateModeRenderingOn
    
vtkActor head
    head SetMapper mapper
    eval [head GetProperty] SetColor $raw_sienna

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor head
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] SetPosition 99.8847 537.86 22.4716 
[ren1 GetActiveCamera] SetFocalPoint 99.8847 109.81 15 
[ren1 GetActiveCamera] SetViewAngle 20
[ren1 GetActiveCamera] SetViewUp 0 1 0

ren1 ResetCameraClippingRange

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

#renWin SetFileName "edgePoints.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
