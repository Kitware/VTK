catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Source the interactor that we will use for the TkRenderWidget
source $VTK_TCL/../graphics/examplesTcl/TkInteractor.tcl

# user interface command widget
source $VTK_TCL/vtkInt.tcl


vtkSphereSource sphere
  sphere SetThetaResolution 6
  sphere SetPhiResolution 5

vtkQuadricDecimation deci
  deci SetInput [sphere GetOutput]
# max cost set to large number to ensure that we reach the maximum number of
# collapsed edges we set
  deci SetMaximumCost 1000
  deci SetMaximumCollapsedEdges 1
  deci DebugOn

# ------------------- Create the UI ---------------------

# UI Variables
set ThetaRes 6
set PhiRes 5
set MaxCollapsedEdges 1



# prevent the tk window from showing up then start the event loop
wm withdraw .

# Create the toplevel window
toplevel .top
wm title .top {Testing vtkQuadricDecimation}

# Create some frames
frame .top.f1 
frame .top.f2
pack .top.f1 .top.f2 -side top -expand 1 -fill both

vtkRenderWindow renWin
vtkTkRenderWidget .top.f1.rw -width 400 -height 400 -rw renWin
BindTkRenderWidget .top.f1.rw
pack .top.f1.rw -expand 1 -fill both

# create a rendering window and renderer
vtkRenderer ren1
    renWin AddRenderer ren1

scale .top.f2.s1 -label " Phi Resolution: " -orient horizontal \
	-length 200 -from 3 -to 50 -variable PhiRes 
scale .top.f2.s2 -label " ThetaResolution: " -orient horizontal \
	-length 200 -from 3 -to 50 -variable ThetaRes 
scale .top.f2.s3 -label " EdgeCollapsed: " -orient horizontal \
	-length 200 -from 0 -to 19 -variable MaxCollapsedEdges

pack .top.f2.s1 .top.f2.s2 .top.f2.s3 -side top -expand 1 -fill both

button .top.f2.b1 -text "Quit" -command {exit}
pack .top.f2.b1  -expand 1 -fill x

bind .top.f2.s1 <ButtonRelease> {
   sphere SetPhiResolution $PhiRes
   renWin Render
}

bind .top.f2.s2 <ButtonRelease> { 
   sphere SetThetaResolution $ThetaRes
   renWin Render
}

bind .top.f2.s3 <ButtonRelease> {
   deci SetMaximumCollapsedEdges $MaxCollapsedEdges
   renWin Render
}


vtkPolyDataMapper mapper
  mapper SetInput [deci GetOutput]
vtkActor actor
  actor SetMapper mapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor

ren1 SetBackground 1 1 1
