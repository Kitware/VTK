# Decimate.tcl - a little application to decimate files
# 	Written by Will
#
catch {load vtktcl}

# Source external files
source TkInteractor.tcl
source vtkInt.tcl

# Define global variables
vtkPolyData PolyData
    PolyData GlobalWarningDisplayOff
vtkPolyData PreviousPolyData
vtkPolyData TempPolyData
vtkCellTypes CellTypes
set deciReduction 0.0
set deciPreserve 1
set view Left
set Surface 1
set FEdges 0
set BEdges 0
set NMEdges 0
set Compare 0

######################################## Create top-level GUI
#
wm title . "vtk Decimator"
frame .mbar -relief raised -bd 2
pack .mbar -side top -fill x

menubutton .mbar.file -text File -menu .mbar.file.menu
menubutton .mbar.edit -text Edit -menu .mbar.edit.menu
menubutton .mbar.view -text View -menu .mbar.view.menu
menubutton .mbar.options -text Options -menu .mbar.options.menu
menubutton .mbar.help -text Help -menu .mbar.help.menu
pack .mbar.file .mbar.edit .mbar.view .mbar.options -side left
pack .mbar.help -side right

menu .mbar.file.menu
    .mbar.file.menu add command -label Open -command OpenFile
    .mbar.file.menu add command -label Save -command SaveFile -state disabled
    .mbar.file.menu add command -label Exit -command exit

menu .mbar.edit.menu
    .mbar.edit.menu add command -label Clean -command Clean -state disabled
    .mbar.edit.menu add command -label Decimate -command Decimate -state disabled
    .mbar.edit.menu add command -label Smooth -command Smooth -state disabled
    .mbar.edit.menu add command -label Triangulate -command Triangulate \
	    -state disabled
    .mbar.edit.menu add command -label "Undo/Redo" -command Undo

menu .mbar.view.menu
    .mbar.view.menu add checkbutton -label "Object Surface" -variable Surface\
	    -command UpdateGUI
    .mbar.view.menu add checkbutton -label "Feature Edges" -variable FEdges\
	    -command UpdateGUI
    .mbar.view.menu add checkbutton -label "Boundary Edges" -variable BEdges\
	    -command UpdateGUI
    .mbar.view.menu add checkbutton -label "Non-manifold Edges" -variable NMEdges\
	    -command UpdateGUI
    .mbar.view.menu add separator
    .mbar.view.menu add radiobutton -label Front -variable view -value Front\
            -command {UpdateView 1 0 0 0 1 0}
    .mbar.view.menu add radiobutton -label Back -variable view -value Back\
            -command {UpdateView -1 0 0 0 1 0}
    .mbar.view.menu add radiobutton -label Left -variable view -value Left\
            -command {UpdateView 0 0 1 0 1 0}
    .mbar.view.menu add radiobutton -label Right -variable view -value Right\
            -command {UpdateView 0 0 -1 0 1 0}
    .mbar.view.menu add radiobutton -label Top -variable view -value Top\
            -command {UpdateView 0 1 0 0 0 1}
    .mbar.view.menu add radiobutton -label Bottom -variable view -value Bottom\
            -command {UpdateView 0 -1 0 0 0 1}
    .mbar.view.menu add radiobutton -label Isometric -variable view \
        -value Isometric -command {UpdateView 1 1 1 0 1 0}

menu .mbar.options.menu
    .mbar.options.menu add command -label "Compare Results" -command Compare \
	    -state disabled

menu .mbar.help.menu
    .mbar.help.menu add command -label {Are you kidding?}

# The rendering widget
vtkTkRenderWidget .window -width 300 -height 300 
    BindTkRenderWidget .window
pack .window -side top -anchor nw -padx 3 -pady 3 -fill both -expand 1

vtkCamera camera
vtkLight light
vtkRenderer Renderer
    Renderer SetActiveCamera camera
    Renderer AddLight light
vtkRenderer CompareRenderer
    CompareRenderer SetViewport 0.0 0.0 0.5 1.0
    CompareRenderer SetActiveCamera camera
    CompareRenderer AddLight light
set RenWin [.window GetRenderWindow]
$RenWin AddRenderer Renderer

# Status bar
label .status -text "(No data)"
pack .status -side top -anchor w -expand 1 -fill x

# Procedure to set particular views
proc UpdateView {x y z vx vy vz} {
    set camera [Renderer GetActiveCamera]
    $camera SetViewPlaneNormal $x $y $z
    $camera SetViewUp $vx $vy $vz
    Renderer ResetCamera
    Render
}

# Procedure opens file and resets view
proc OpenFile {} {
    global RenWin

    set types {
        {{BYU}                                  {.g}          }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.vtk}        }
        {{All Files }                           *             }
    }
    set filename [tk_getOpenFile -filetypes $types]
    if { $filename != "" } {
        if { [info commands reader] != "" } {reader Delete}
        if { [string match *.g $filename] } {
            vtkBYUReader reader
            reader SetGeometryFileName $filename
        } elseif { [string match *.stl $filename] } {
            vtkSTLReader reader
            reader SetFileName $filename
        } elseif { [string match *.vtk $filename] } {
            vtkPolyDataReader reader
            reader SetFileName $filename
        } else {
            puts "Can't read this file"
            return
        }

        reader Update
        UpdateUndo "reader"
	UpdateGUI

        Renderer ResetCamera
        $RenWin Render
    }
}

# Procedure saves data to file
proc SaveFile {} {
    global PolyData

    set types {
        {{BYU}                                  {.g}          }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.vtk}        }
        {{All Files }                           *             }
    }
    set filename [tk_getSaveFile -filetypes $types]
    if { $filename != "" } {
        if { [info commands writer] != "" } {writer Delete}
        if { [string match *.g $filename] } {
            vtkBYUWriter writer
            writer SetGeometryFileName $filename
        } elseif { [string match *.stl $filename] } {
            vtkSTLWriter writer
            writer SetFileName $filename
        } elseif { [string match *.vtk $filename] } {
            vtkPolyDataWriter writer
            writer SetFileName $filename
        } else {
            puts "Can't write this file"
            return
        }
        
        writer SetInput PolyData
        writer Write
    }
}

# Enable the undo procedure after filter execution
proc UpdateUndo {filter} {

    PreviousPolyData CopyStructure PolyData
    [PreviousPolyData GetPointData] PassData [PolyData GetPointData]
    PreviousPolyData Modified

    PolyData CopyStructure [$filter GetOutput]
    [PolyData GetPointData] PassData [[$filter GetOutput] GetPointData]
    PolyData Modified
}

# Undo last edit
proc Undo {} {
    global RenWin

    TempPolyData CopyStructure PolyData
    [TempPolyData GetPointData] PassData [PolyData GetPointData]

    PolyData CopyStructure PreviousPolyData
    [PolyData GetPointData] PassData [PreviousPolyData GetPointData]
    PolyData Modified

    PreviousPolyData CopyStructure TempPolyData
    [PreviousPolyData GetPointData] PassData [TempPolyData GetPointData]
    PreviousPolyData Modified

    UpdateGUI
    $RenWin Render
}

# Create pipeline
vtkPolyDataMapper   mapper
    mapper SetInput PolyData
vtkActor actor
    actor SetMapper mapper

# Welcome banner
vtkVectorText banner
    banner SetText "     vtk Decimator"
vtkPolyDataMapper bannerMapper
    bannerMapper SetInput [banner GetOutput]
vtkActor bannerActor
    bannerActor SetMapper bannerMapper

# Actor used for side-by-side data comparison
vtkPolyDataMapper CompareMapper
    CompareMapper SetInput PreviousPolyData
vtkActor CompareActor
    CompareActor SetMapper CompareMapper
CompareRenderer AddActor CompareActor

# Edges
vtkFeatureEdges FeatureEdges
    FeatureEdges SetInput PolyData
    FeatureEdges BoundaryEdgesOff
    FeatureEdges NonManifoldEdgesOff
    FeatureEdges ManifoldEdgesOff
    FeatureEdges FeatureEdgesOff
vtkPolyDataMapper FEdgesMapper
    FEdgesMapper SetInput [FeatureEdges GetOutput]
vtkActor FEdgesActor
    FEdgesActor SetMapper FEdgesMapper

Renderer AddActor bannerActor
[Renderer GetActiveCamera] Zoom 1.25


# Procedure updates data statistics and GUI menus
#
proc UpdateGUI {} {
    global Surface RenWin
    global FEdges BEdges NMEdges RenWin

    set NumberOfNodes [PolyData GetNumberOfPoints]
    set NumberOfElements [PolyData GetNumberOfCells]
    PolyData GetCellTypes CellTypes

    Renderer RemoveActor bannerActor
    Renderer RemoveActor actor
    Renderer RemoveActor FEdgesActor

    # Check to see whether to add surface model
    if { [PolyData GetNumberOfCells] <= 0 } {
	Renderer AddActor bannerActor
	.mbar.edit.menu entryconfigure 1 -state disabled
	.mbar.edit.menu entryconfigure 2 -state disabled
	.mbar.edit.menu entryconfigure 3 -state disabled
	.mbar.file.menu entryconfigure 1 -state disabled
	.mbar.options.menu entryconfigure 1 -state disabled
        set s "(None)"

    } else {
	if { $Surface } {Renderer AddActor actor}

	if { $FEdges || $BEdges || $NMEdges } {
	    Renderer AddActor FEdgesActor
	    FeatureEdges SetBoundaryEdges $BEdges
	    FeatureEdges SetFeatureEdges $FEdges
	    FeatureEdges SetNonManifoldEdges $NMEdges
	}

	.mbar.edit.menu entryconfigure 1 -state normal
        if { [CellTypes GetNumberOfTypes] != 1 || [CellTypes GetCellType 0] != 5 } {
	    .mbar.edit.menu entryconfigure 2 -state disabled
            set s [format "Vertices:%d    Cells:%d" \
                   $NumberOfNodes $NumberOfElements]
	} else {
            .mbar.edit.menu entryconfigure 2 -state normal
            set s [format "Vertices:%d    Triangles:%d" \
                   $NumberOfNodes $NumberOfElements]
	}
	.mbar.edit.menu entryconfigure 3 -state normal
	.mbar.edit.menu entryconfigure 4 -state normal
	.mbar.file.menu entryconfigure 2 -state normal
	.mbar.options.menu entryconfigure 1 -state normal
    }

    .status configure -text $s
}

### Procedure manages splitting screen and comparing data
#
proc Compare {} {
    global Compare RenWin

    if { $Compare == 0} {
	$RenWin AddRenderer CompareRenderer
	Renderer SetViewport 0.5 0.0 1.0 1.0
	.mbar.options.menu entryconfigure 1 -label "Uncompare Results"
	set Compare 1

    } else {
	$RenWin RemoveRenderer CompareRenderer
	Renderer SetViewport 0.0 0.0 1.0 1.0
	.mbar.options.menu entryconfigure 1 -label "Compare Results"
	set Compare 0
    }

    $RenWin Render
}

########################## The decimation GUI
#
# Procedure defines GUI and behavior for decimating data
#
proc Decimate {} {
    UpdateDecimationGUI
    wm deiconify .decimate
}

proc CloseDecimate {} {
    wm withdraw .decimate
}

toplevel .decimate
wm withdraw .decimate
wm title .decimate "Decimate"
wm protocol .decimate WM_DELETE_WINDOW {wm withdraw .decimate}

frame .decimate.f1
checkbutton .decimate.f1.preserve -variable deciPreserve \
	-text "Preserve Topology"
scale .decimate.f1.red -label "Requested Number Of Polygons" \
	-from 0 -to 100000 -length 3.0i -orient horizontal \
	-resolution 1 -command SetDeciPolygons
.decimate.f1.red set 4000
pack .decimate.f1.preserve .decimate.f1.red \
	-pady 0.1i -side top -anchor w

frame .decimate.fb
button .decimate.fb.apply -text Apply -command ApplyDecimation
button .decimate.fb.cancel -text Cancel -command CloseDecimate
pack .decimate.fb.apply .decimate.fb.cancel -side left -expand 1 -fill x
pack .decimate.f1 .decimate.fb -side top -fill both -expand 1

vtkDecimatePro deci

proc UpdateDecimationGUI {} {

   set numPolys [PolyData GetNumberOfCells]
   .decimate.f1.red configure -to $numPolys

   SetDeciPolygons [.decimate.f1.red get]
}

proc ApplyDecimation {} {
    global deciReduction deciPreserve RenWin

    deci SetInput PolyData    

    deci SetTargetReduction $deciReduction
    deci SetPreserveTopology $deciPreserve
    deci Update

    UpdateUndo "deci"
    UpdateGUI

    $RenWin Render
    CloseDecimate
}  

proc SetDeciPolygons value {
    global deciReduction

    set numInPolys [PolyData GetNumberOfCells]
    if { $numInPolys <= 0 } {return}
    set deciReduction [expr (double($numInPolys) - $value) / $numInPolys]
}

########################## The smooth poly data GUI
#
# Procedure defines GUI and behavior for decimating data
#
proc Smooth {} {
    UpdateSmoothGUI
    wm deiconify .smooth
}

proc CloseSmooth {} {
    wm withdraw .smooth
}

toplevel .smooth
wm withdraw .smooth
wm title .smooth "Smooth"
wm protocol .smooth WM_DELETE_WINDOW {wm withdraw .smooth}

frame .smooth.f1
scale .smooth.f1.num -label "Number Of Iterations" \
	-from 1 -to 1000 -length 3.0i -orient horizontal \
	-resolution 1
.smooth.f1.num set 100
scale .smooth.f1.fact -label "RelaxationFactor" \
	-from 0.00 -to 1.00 -length 3.0i -orient horizontal \
	-resolution 0.01
.smooth.f1.fact set 0.01
pack .smooth.f1.num .smooth.f1.fact \
	-pady 0.1i -side top -anchor w

frame .smooth.fb
button .smooth.fb.apply -text Apply -command ApplySmooth
button .smooth.fb.cancel -text Cancel -command CloseSmooth
pack .smooth.fb.apply .smooth.fb.cancel -side left -expand 1 -fill x
pack .smooth.f1 .smooth.fb -side top -fill both -expand 1

vtkSmoothPolyDataFilter smooth

proc UpdateSmoothGUI {} {

   .smooth.f1.num set [smooth GetNumberOfIterations]
   .smooth.f1.fact set [smooth GetRelaxationFactor]
}

proc ApplySmooth {} {
    global RenWin

    smooth SetInput PolyData    

    smooth SetNumberOfIterations [.smooth.f1.num get]
    smooth SetRelaxationFactor [.smooth.f1.fact get]
    smooth Update

    UpdateUndo "smooth"
    UpdateGUI

    $RenWin Render
    CloseSmooth
}  

proc SetDeciPolygons value {
    global deciReduction

    set numInPolys [PolyData GetNumberOfCells]
    if { $numInPolys <= 0 } {return}
    set deciReduction [expr (double($numInPolys) - $value) / $numInPolys]
}

########################## The clean GUI
#
# Procedure defines GUI and behavior for cleaning data. Cleaning means
# removing degenerate polygons and eliminating coincident or unused points.

proc Clean {} {
    UpdateCleanGUI
    wm deiconify .clean
}
proc CloseClean {} {
    wm withdraw .clean
}

toplevel .clean
wm withdraw .clean
wm title .clean "Clean Data"
wm protocol .clean WM_DELETE_WINDOW {wm withdraw .clean}

frame .clean.f1
scale .clean.f1.s -label "Tolerance" \
	-from 0.000 -to 1.000 -length 3.0i -orient horizontal\
	-resolution 0.001 -digits 3
.clean.f1.s set 0.000
pack .clean.f1.s -side top -anchor w

frame .clean.fb
button .clean.fb.apply -text Apply -command ApplyClean
button .clean.fb.cancel -text Cancel -command CloseClean
pack .clean.fb.apply .clean.fb.cancel -side left -expand 1 -fill x
pack .clean.f1 .clean.fb -side top -fill both -expand 1

vtkCleanPolyData cleaner

proc UpdateCleanGUI {} {
    .clean.f1.s set [cleaner GetTolerance]
}

proc ApplyClean {} {
    global RenWin

    cleaner SetInput PolyData
    cleaner SetTolerance [.clean.f1.s get]
    cleaner Update

    UpdateUndo "cleaner"
    UpdateGUI

    $RenWin Render
    CloseClean
}
########################## The triangulate GUI
#
# Procedure defines GUI and behavior for triangulating data. This will
# convert all polygons into triangles
proc Triangulate {} {
    UpdateTriGUI
    wm deiconify .tri
}
proc CloseTri {} {
    wm withdraw .tri
}

toplevel .tri
wm withdraw .tri
wm title .tri "Triangulate Data"
wm protocol .tri WM_DELETE_WINDOW {wm withdraw .tri}

frame .tri.fb
button .tri.fb.apply -text Apply -command ApplyTri
button .tri.fb.cancel -text Cancel -command CloseTri
pack .tri.fb.apply .tri.fb.cancel -side left -expand 1 -fill x
pack .tri.fb -side top -fill both -expand 1

vtkTriangleFilter tri

proc UpdateTriGUI {} {
}

proc ApplyTri {} {
    global RenWin

    tri SetInput PolyData
    tri Update

    UpdateUndo "tri"
    UpdateGUI

    $RenWin Render
    CloseTri
}
