package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderer ren4
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4

vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create the data -- a plane with a couple of bumps
#
vtkPlaneSource plane
    plane SetXResolution 10
    plane SetYResolution 10

vtkTriangleFilter tf
    tf SetInputConnection [plane GetOutputPort]

# This filter modifies the point coordinates in a couple of spots
#
vtkProgrammableFilter adjustPoints
   adjustPoints SetInputConnection [tf GetOutputPort]
   adjustPoints SetExecuteMethod adjustPointsProc

# The SetExecuteMethod takes a Tcl proc as an argument
# In here is where all the processing is done.
#
proc adjustPointsProc {} {
    set input [adjustPoints GetPolyDataInput]
    set inPts [$input GetPoints]
    set numPts [$input GetNumberOfPoints]
    vtkPoints newPts
    newPts SetNumberOfPoints $numPts

    for {set i 0} {$i < $numPts} {incr i} {
	eval newPts SetPoint $i [$inPts GetPoint $i]
    }

    set pt [$inPts GetPoint 17]
    newPts SetPoint 17 [lindex $pt 0] [lindex $pt 1] 0.25

    set pt [$inPts GetPoint 50]
    newPts SetPoint 50 [lindex $pt 0] [lindex $pt 1] 1.0

    set pt [$inPts GetPoint 77]
    newPts SetPoint 77 [lindex $pt 0] [lindex $pt 1] 0.125

    [adjustPoints GetPolyDataOutput] CopyStructure $input
    [adjustPoints GetPolyDataOutput] SetPoints newPts

    newPts Delete; #reference counting - it's ok
}

# Now remove the extreme peak in the center
vtkGeometryFilter gf
    gf SetInput [adjustPoints GetPolyDataOutput]
    gf ExtentClippingOn
    gf SetExtent -100 100 -100 100 -1 0.9

# Create a table of decimation conditions
#
set boundaryVertexDeletion "On Off"
set accumulates "On Off"
foreach topology $boundaryVertexDeletion {
  foreach accumulate $accumulates {
    vtkDecimatePro deci$topology$accumulate
      deci$topology$accumulate SetInputConnection [gf GetOutputPort]
      deci$topology$accumulate SetTargetReduction .95
      deci$topology$accumulate BoundaryVertexDeletion$topology
      deci$topology$accumulate AccumulateError$accumulate
    vtkPolyDataMapper mapper$topology$accumulate
      mapper$topology$accumulate SetInputConnection [deci$topology$accumulate GetOutputPort]
    vtkActor plane$topology$accumulate
      plane$topology$accumulate SetMapper mapper$topology$accumulate
  }
}

# Add the actors to the renderer, set the background and size
#
ren1 SetViewport 0 .5 .5 1
ren2 SetViewport .5 .5 1 1
ren3 SetViewport 0 0 .5 .5
ren4 SetViewport .5 0 1 .5

ren1 AddActor planeOnOn
ren2 AddActor planeOnOff
ren3 AddActor planeOffOn
ren4 AddActor planeOffOff

vtkCamera camera
ren1 SetActiveCamera camera
ren2 SetActiveCamera camera
ren3 SetActiveCamera camera
ren4 SetActiveCamera camera

[ren1 GetActiveCamera] SetPosition -0.128224 0.611836 2.31297
[ren1 GetActiveCamera] SetFocalPoint 0 0 0.125
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp  0.162675 0.952658 -0.256864

ren1 SetBackground 0 0 0
ren2 SetBackground 0 0 0
ren3 SetBackground 0 0 0
ren4 SetBackground 0 0 0

renWin SetSize 500 500

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


