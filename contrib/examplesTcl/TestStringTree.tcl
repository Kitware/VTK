# A simple script that tests the vtkTree object.
# Directory structure is traversed and converted to a vtkStringTree object.
# The tree object is then printed out in the interactor.


# get the interactor ui
source ../../examplesTcl/vtkInt.tcl



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
wm deiconify .vtkInteract
dovtk "dirTree Print" .vtkInteract
