# This test does the same thing as ManyActors.tcl, 
# but it creates only one actor.




catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }





# user interface command widget
source $VTK_TCL/vtkInt.tcl


# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn  
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin



vtkPolyData pd
vtkPoints pts
vtkCellArray verts

for {set i 0} {$i < 10} {incr i} {
    for {set j 0} {$j < 10} {incr j} {
	for {set k 0} {$k < 10} {incr k} {
	    set id [pts InsertNextPoint $i $j $k]
	    verts InsertNextCell 1
	    verts InsertCellPoint $id
	}
    }
}


pd SetPoints pts
pd SetVerts verts






vtkPolyDataMapper mapper
  mapper SetInput pd
vtkActor actor
  actor SetMapper mapper

# assign our actor to the renderer
ren1 AddActor actor

# This is not necessary for this script, 
# but to keep it similiar to ManyActors.tcl ...

# culler will remove points unless 
# we make the minimum coverage less than 0.
set cullers [ren1 GetCullers]
$cullers InitTraversal
set culler [$cullers GetNextItem]
$culler SetMinimumCoverage -0.01 



proc GetTime {} {
    return [ren1 GetLastRenderTimeInSeconds]
}

set cam [ren1 GetActiveCamera]

set count 0
set totalTime 0.0


# lets loop through a little animation to time the render.
for {set i 0} {$i < 200} {incr i} {
  renWin Render
  incr count
  set totalTime [expr $totalTime + [ren1 GetLastRenderTimeInSeconds]]
  $cam Azimuth [expr 0.04 * (150 - $i)]
  $cam Elevation [expr 0.04 * ($i - 50)]
  $cam OrthogonalizeViewUp
  }



# I am worried about numerical prescision 
# causing regression test to fail.

$cam SetFocalPoint 0 0 0
$cam SetPosition 0 1 0
$cam SetViewUp 0 0 1

ren1 ResetCamera
renWin Render




# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



#tk_messageBox -message "Average frame rate = [expr 1.0 * $count / $totalTime]"


