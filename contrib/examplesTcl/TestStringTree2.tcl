catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl






# A simple script that tests the vtkTree object.
# Directory structure is traversed and converted to a vtkStringTree object.

# this test is for coverage test (which needs an image)


# move to root vtk directory
cd ..

# initialize the directory list

set entry [pwd]
vtkStringTree dirTree
dirTree SetItem [file tail $entry]



proc DirGrow {t e} {
    # assumes e is a directory
    set entryList [glob -nocomplain $e/*]
    foreach entry $entryList {
	if {[file isdirectory $entry]} {
	    set idx [$t AddNewNode [file tail $entry]]
	    $t MoveToChild $idx
	    DirGrow $t $entry
	    $t MoveToParent
	} else {
	    set idx [$t AddNewLeaf [file tail $entry]]
	}
    }
}


if {[file isdirectory $entry]} {
    DirGrow dirTree $entry
}


wm withdraw .
#wm deiconify .vtkInteract
#dovtk "dirTree Print" .vtkInteract





# draw something for regression tests

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkConeSource cone
    cone SetResolution 6
    cone SetRadius .1

vtkPolyDataMapper hairMapper
  hairMapper SetInput [cone GetOutput]

vtkActor hair
  hair SetMapper hairMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor hair
[ren1 GetActiveCamera] Dolly 2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30

eval [hair GetProperty] SetDiffuseColor $saddle_brown

renWin SetSize 320 240
ren1 SetBackground .1 .2 .4

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName OBJReader.tcl.ppm
#renWin SaveImageAsPPM


