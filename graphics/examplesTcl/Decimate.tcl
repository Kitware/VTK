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
    .mbar.file.menu add command -label Save -command SaveFile
    .mbar.file.menu add command -label Exit -command exit

menu .mbar.edit.menu
    .mbar.edit.menu add command -label Clean -command Clean
    .mbar.edit.menu add command -label Decimate -command Decimate
    .mbar.edit.menu add command -label Triangulate -command Triangulate
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

# Status bar
label .status -text "(No data)"
pack .status -side top -anchor w -expand 1 -fill x

# Procedure to set particular views
proc UpdateView {x y z vx vy vz} {
    global ren renWin

    set camera [$ren GetActiveCamera]
    $camera SetViewPlaneNormal $x $y $z
    $camera SetViewUp $vx $vy $vz
    $ren ResetCamera
    Render
}

# Procedure opens file and resets view
proc OpenFile {} {
    global ren renWin

    set types {
        {{BYU}                                  {.g}          }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.vtk}        }
        {{All Files }                           *             }
    }
    set filename [tk_getOpenFile -filetypes $types]
    if { $filename != "" } {
        $ren RemoveActor bannerActor
        $ren RemoveActor actor
        $ren RemoveActor FEdgesActor
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

        PolyData CopyStructure [reader GetOutput]
        [PolyData GetPointData] PassData [[reader GetOutput] GetPointData]
        mapper SetInput PolyData

        if { [[reader GetOutput] GetNumberOfCells] <= 0 } {
            $ren AddActor bannerActor
        } else {
            $ren AddActor actor
            UpdateFEdges
        }
        .mbar.file.menu activate 2        
        $ren ResetCamera
        $renWin Render

	UpdateStatistics
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

set renWin [.window GetRenderWindow]
set ren   [$renWin MakeRenderer]

$ren AddActor bannerActor
[$ren GetActiveCamera] Zoom 1.25

# Procedure manages surface display
proc UpdateSurface {} {
    global ren renWin
    global Surface

   if { ! $Surface } {
	$ren RemoveActor actor
   } else {
        $ren AddActor actor
   }
   $renWin Render
}

# Procedure manages feature edge on/off
proc UpdateFEdges {} {
    global ren renWin
    global FEdges BEdges NMEdges

   if { ! $FEdges && ! $BEdges && ! $NMEdges } {
	$ren RemoveActor FEdgesActor
   } else {
        set actors [$ren GetActors]
        if { [$actors GetNumberOfItems] < 2 } {$ren AddActor FEdgesActor}
	FeatureEdges SetBoundaryEdges $BEdges
	FeatureEdges SetFeatureEdges $FEdges
	FeatureEdges SetNonManifoldEdges $NMEdges
   }
   $renWin Render
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
    global deciReduction deciPreserve renWin

    deci SetInput PolyData    

    deci SetTargetReduction $deciReduction
    deci SetPreserveTopology $deciPreserve
    deci Update

    UpdateUndo "deci"

    $renWin Render
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
    global renWin

    cleaner SetInput PolyData
    cleaner SetTolerance [.clean.f1.s get]
    cleaner Update

    UpdateUndo "cleaner"

    $renWin Render
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
    global renWin

    tri SetInput PolyData
    tri Update

    UpdateUndo "tri"

    $renWin Render
    UpdateStatistics
    CloseTri
}
