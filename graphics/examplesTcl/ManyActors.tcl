# This test creates 1000 actors and renders an animation.




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


for {set i 0} {$i < 10} {incr i} {
    for {set j 0} {$j < 10} {incr j} {
	for {set k 0} {$k < 10} {incr k} {

	    vtkPolyData pd$i$j$k
	    vtkPoints pts$i$j$k
	    vtkCellArray verts$i$j$k

	    set id [pts$i$j$k InsertNextPoint $i $j $k]
	    verts$i$j$k InsertNextCell 1
	    verts$i$j$k InsertCellPoint $id

	    pd$i$j$k SetPoints pts$i$j$k
	    pd$i$j$k SetVerts verts$i$j$k
	    pd$i$j$k Squeeze

	    vtkPolyDataMapper mapper$i$j$k
	    mapper$i$j$k ImmediateModeRenderingOn
	    mapper$i$j$k SetInput pd$i$j$k
	    vtkActor actor$i$j$k
	    actor$i$j$k SetMapper mapper$i$j$k
	    ren1 AddActor actor$i$j$k

	}
    }
}

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

# lets loop through a little animation to time the render.
for {set i 0} {$i < 200} {incr i} {
  renWin Render
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



