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
set deciReduction 0.0
set deciPreserve 1
set view Left
set Surface 1
set FEdges 0
set BEdges 0
set NMEdges 0

######################################## Create top-level GUI
#
wm title . "vtk Decimator"
frame .mbar -relief raised -bd 2
pack .mbar -side top -fill x

menubutton .mbar.file -text File -menu .mbar.file.menu
menubutton .mbar.edit -text Edit -menu .mbar.edit.menu
menubutton .mbar.view -text View -menu .mbar.view.menu
menubutton .mbar.help -text Help -menu .mbar.help.menu
pack .mbar.file .mbar.edit .mbar.view -side left
pack .mbar.help -side right

menu .mbar.file.menu
    .mbar.file.menu add command -label Open -command OpenFile
    .mbar.file.menu add command -label Save -command SaveFile -state disabled
    .mbar.file.menu add command -label Exit -command exit

menu .mbar.edit.menu
    .mbar.edit.menu add command -label Clean -command Clean -state disabled
    .mbar.edit.menu add command -label Decimate -command Decimate -state disabled
    .mbar.edit.menu add command -label Triangulate -command Triangulate \
	    -state disabled
    .mbar.edit.menu add command -label "Undo/Redo" -command Undo

menu .mbar.view.menu
    .mbar.view.menu add checkbutton -label "Object Surface" -variable Surface\
	-command UpdateSurface
    .mbar.view.menu add checkbutton -label "Feature Edges" -variable FEdges\
	-command UpdateFEdges
    .mbar.view.menu add checkbutton -label "Boundary Edges" -variable BEdges\
	-command UpdateFEdges
    .mbar.view.menu add checkbutton -label "Non-manifold Edges" -variable NMEdges\
	-command UpdateFEdges
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

menu .mbar.help.menu
    .mbar.help.menu add command -label {Are you kidding?}

# The rendering widget
vtkTkRenderWidget .window -width 300 -height 300 
    BindTkRenderWidget .window
pack .window -side top -anchor nw -padx 3 -pady 3 -fill both -expand 1

vtkRenderer Renderer
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
        Renderer RemoveActor bannerActor
        Renderer RemoveActor actor
        Renderer RemoveActor FEdgesActor
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

        PreviousPolyData CopyStructure [reader GetOutput]
        [PreviousPolyData GetPointData] PassData [[reader GetOutput] GetPointData]
        PolyData CopyStructure [reader GetOutput]
        [PolyData GetPointData] PassData [[reader GetOutput] GetPointData]

        UpdateMenus

        Renderer ResetCamera
        $RenWin Render

	UpdateStatistics
    }
}

# Procedure updates the menus, actors, etc. after a read occurs
proc UpdateMenus {} {

    if { [PolyData GetNumberOfCells] <= 0 } {
	Renderer AddActor bannerActor
	.mbar.edit.menu entryconfigure 1 -state disabled
	.mbar.edit.menu entryconfigure 2 -state disabled
	.mbar.edit.menu entryconfigure 3 -state disabled
	.mbar.file.menu entryconfigure 1 -state disabled
    } else {
	Renderer AddActor actor
	UpdateFEdges
	.mbar.edit.menu entryconfigure 1 -state normal
	.mbar.edit.menu entryconfigure 2 -state normal
	.mbar.edit.menu entryconfigure 3 -state normal
	.mbar.file.menu entryconfigure 2 -state normal
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
    PolyData CopyStructure [$filter GetOutput]
    [PolyData GetPointData] PassData [[$filter GetOutput] GetPointData]
}

# Undo last edit
proc Undo {} {
    TempPolyData CopyStructure PolyData
    [TempPolyData GetPointData] PassData [PolyData GetPointData]

    PolyData CopyStructure PreviousPolyData
    [PolyData GetPointData] PassData [PreviousPolyData GetPointData]
    PolyData Modified

    PreviousPolyData CopyStructure TempPolyData
    [PreviousPolyData GetPointData] PassData [TempPolyData GetPointData]

    UpdateStatistics
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

# Procedure manages surface display
proc UpdateSurface {} {
    global Surface RenWin

    if { ! $Surface } {
	Renderer RemoveActor actor
    } else {
        Renderer AddActor actor
    }
    $RenWin Render
}

# Procedure manages feature edge on/off
proc UpdateFEdges {} {
    global FEdges BEdges NMEdges RenWin

    if { ! $FEdges && ! $BEdges && ! $NMEdges } {
	Renderer RemoveActor FEdgesActor
    } else {
        set actors [Renderer GetActors]
        if { [$actors GetNumberOfItems] < 2 } {Renderer AddActor FEdgesActor}
	FeatureEdges SetBoundaryEdges $BEdges
	FeatureEdges SetFeatureEdges $FEdges
	FeatureEdges SetNonManifoldEdges $NMEdges
    }
    $RenWin Render
}

# Procedure updates data statistics
#
proc UpdateStatistics {} {

    set NumberOfNodes [PolyData GetNumberOfPoints]
    set NumberOfElements [PolyData GetNumberOfCells]

    set s [format "Vertices:%d    Polygons:%d" \
	$NumberOfNodes $NumberOfElements]

    .status configure -text $s
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

    $RenWin Render
    UpdateStatistics
    CloseDecimate
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

    $RenWin Render
    UpdateStatistics
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

    $RenWin Render
    UpdateStatistics
    CloseTri
}
