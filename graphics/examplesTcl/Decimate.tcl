# Decimate.tcl - a little application to decimate files
# 	Written by Will
#
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# Source external files
source TkInteractor.tcl
source $VTK_TCL/vtkInt.tcl

# Define global variables
set deciReduction 0.0
set deciPreserve 1
set view Left
set Surface 1
set FEdges 0
set BEdges 0
set NMEdges 0
set Compare 0
set edgeSplitting 1
set flipNormals 0

# Instances of vtk objects
vtkPolyData PolyData
#    PolyData GlobalWarningDisplayOff
vtkPolyData PreviousPolyData
vtkPolyData TempPolyData
vtkCellTypes CellTypes

vtkDecimatePro deci
    deci SetStartMethod {StartProgress deci "Decimating..."}
    deci SetProgressMethod {ShowProgress deci "Decimating..."}
    deci SetEndMethod EndProgress
vtkSmoothPolyDataFilter smooth
    smooth SetStartMethod {StartProgress smooth "Smoothing..."}
    smooth SetProgressMethod {ShowProgress smooth "Smoothing..."}
    smooth SetEndMethod EndProgress
vtkCleanPolyData cleaner
    cleaner SetStartMethod {StartProgress cleaner "Cleaning..."}
    cleaner SetProgressMethod {ShowProgress cleaner "Cleaning..."}
    cleaner SetEndMethod EndProgress
vtkPolyDataConnectivityFilter connect
    connect SetStartMethod {StartProgress connect "Connectivity..."}
    connect SetProgressMethod {ShowProgress connect "Connectivity..."}
    connect SetEndMethod EndProgress
vtkTriangleFilter tri
    tri SetStartMethod {StartProgress tri "Triangulating..."}
    tri SetProgressMethod {ShowProgress tri "Triangulating..."}
    tri SetEndMethod EndProgress
vtkPolyDataNormals normals
    normals SetStartMethod {StartProgress normals "Generating Normals..."}
    normals SetProgressMethod {ShowProgress normals "Generating Normals..."}
    normals SetEndMethod EndProgress

######################################## Create top-level GUI
#
wm title . "vtk Decimator v1.0"
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
    .mbar.edit.menu add command -label Connectivity -command Connect -state disabled
    .mbar.edit.menu add command -label Decimate -command Decimate -state disabled
    .mbar.edit.menu add command -label Normals -command Normals -state disabled
    .mbar.edit.menu add command -label Smooth -command Smooth -state disabled
    .mbar.edit.menu add command -label Triangulate -command Triangulate \
	    -state disabled
    .mbar.edit.menu add command -label "Undo/Redo" -command Undo

menu .mbar.view.menu
    .mbar.view.menu add checkbutton -label "Object Surface" -variable Surface\
	  -command {UpdateGUI; $RenWin Render}
    .mbar.view.menu add checkbutton -label "Feature Edges" -variable FEdges\
	  -command {UpdateGUI; $RenWin Render}
    .mbar.view.menu add checkbutton -label "Boundary Edges" -variable BEdges\
	  -command {UpdateGUI; $RenWin Render}
    .mbar.view.menu add checkbutton -label "Non-manifold Edges" -variable NMEdges\
	  -command {UpdateGUI; $RenWin Render}
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
    .mbar.options.menu add command -label "Background Color..." \
	    -command BackgroundColor
    .mbar.options.menu add command -label "Surface Properties..." \
	    -command Properties

menu .mbar.help.menu
    .mbar.help.menu add command -label {Buy a Kitware support contract}

# The rendering widget
vtkTkRenderWidget .window -width 300 -height 300 
    BindTkRenderWidget .window
pack .window -side top -anchor nw -padx 3 -pady 3 -fill both -expand 1

# Status bar
frame .bottomF -relief sunken -borderwidth 3
label .bottomF.status -text "(No data)" -borderwidth 0
pack .bottomF.status -side top -anchor w -expand 1 -fill x -padx 0 -pady 0
pack .bottomF  -side top -anchor w -expand 1 -fill x -padx 0 -pady 0

# Graphics objects
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

# Procedure to set particular views
proc UpdateView {x y z vx vy vz} {
    global RenWin

    set camera [Renderer GetActiveCamera]
    $camera SetDirectionOfProjection [expr -$x] [expr -$y] [expr -$z]
    $camera SetViewUp $vx $vy $vz
    Renderer ResetCamera
    Render $RenWin
}

# Procedure opens file and resets view
proc OpenFile {} {
    global RenWin CurrentFilter

    set types {
        {{BYU}                                  {.g}          }
        {{Cyberware (Laser Scanner)}            {.cyb}        }
        {{Marching Cubes}                       {.tri}        }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.vtk}        }
        {{Wavefront}                            {.obj}        }
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
        } elseif { [string match *.cyb $filename] } {
            vtkCyberReader reader
            reader SetFileName $filename
        } elseif { [string match *.tri $filename] } {
            vtkMCubesReader reader
            reader SetFileName $filename
        } elseif { [string match *.obj $filename] } {
            vtkOBJReader reader
            reader SetFileName $filename
        } else {
            puts "Can't read this file"
            return
        }

	reader SetStartMethod {StartProgress reader "Reading..."}
	reader SetProgressMethod {ShowProgress reader "Reading..."}
	reader SetEndMethod EndProgress

        UpdateUndo "reader"
	UpdateGUI

	set filename "vtk Decimator: [file tail $filename]"
	wm title . $filename

        Renderer ResetCamera
        $RenWin Render
    }
}

# Procedure saves data to file
proc SaveFile {} {
    global PolyData RenWin

    set types {
        {{BYU}                                  {.g}          }
        {{Marching Cubes}                       {.tri}        }
        {{RIB (Renderman)}                      {.rib}        }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.vtk}        }
        {{VRML}                                 {.wrl}        }
        {{Wavefront OBJ}                            {.obj}        }
        {{All Files }                           *             }
    }
    set filename [tk_getSaveFile -filetypes $types]
    if { $filename != "" } {
        if { [info commands writer] != "" } {writer Delete}
        if { [string match *.g $filename] } {
            vtkBYUWriter writer
            writer SetGeometryFileName $filename
            writer SetInput PolyData
        } elseif { [string match *.stl $filename] } {
            vtkSTLWriter writer
            writer SetFileName $filename
            writer SetInput PolyData
        } elseif { [string match *.vtk $filename] } {
            vtkPolyDataWriter writer
            writer SetFileName $filename
            writer SetInput PolyData
        } elseif { [string match *.tri $filename] } {
            vtkMCubesWriter writer
            writer SetFileName $filename
            writer SetInput PolyData
        } elseif { [string match *.wrl $filename] } {
            vtkVRMLExporter writer
	    writer SetRenderWindow $RenWin
            writer SetFileName $filename
        } elseif { [string match *.obj $filename] } {
            vtkOBJExporter writer
	    writer SetRenderWindow $RenWin
            writer SetFilePrefix [file rootname $filename]
        } elseif { [string match *.rib $filename] } {
            vtkRIBExporter writer
	    writer SetRenderWindow $RenWin
            writer SetFilePrefix [file rootname $filename]
        } else {
            puts "Can't write this file"
            return
        }
        
        writer Write
    }
}

# Enable the undo procedure after filter execution
proc UpdateUndo {filter} {
    global CurrentFilter

    set CurrentFilter $filter
    $filter Update

    PreviousPolyData CopyStructure PolyData
    [PreviousPolyData GetPointData] PassData [PolyData GetPointData]
    PreviousPolyData Modified

    PolyData CopyStructure [$filter GetOutput]
    [PolyData GetPointData] PassData [[$filter GetOutput] GetPointData]
    PolyData Modified

    ReleaseData
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

### Procedure initializes filters so that they release their memory
#
proc ReleaseData {} {
    [deci GetOutput] Initialize
    [smooth GetOutput] Initialize
    [cleaner GetOutput] Initialize
    [connect GetOutput] Initialize
    [tri GetOutput] Initialize
    [smooth GetOutput] Initialize
}

#### Create pipeline
vtkPolyDataMapper   mapper
    mapper SetInput PolyData
vtkProperty property
    property SetColor 0.8900 0.8100 0.3400
    property SetSpecularColor 1 1 1
    property SetSpecular 0.3
    property SetSpecularPower 20
    property SetAmbient 0.2
    property SetDiffuse 0.8
vtkActor actor
    actor SetMapper mapper
    actor SetProperty property

# Welcome banner
vtkTextMapper banner
    banner SetInput "vtk Decimator\nVersion 1.0"
    banner SetFontFamilyToArial
    banner SetFontSize 18
    banner ItalicOn
    banner SetJustificationToCentered
vtkActor2D bannerActor
    bannerActor SetMapper banner
    [bannerActor GetProperty] SetColor 0 1 0
    [bannerActor GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [bannerActor GetPositionCoordinate] SetValue 0.5 0.5
Renderer AddProp bannerActor

# Actor used for side-by-side data comparison
vtkPolyDataMapper CompareMapper
    CompareMapper SetInput PreviousPolyData
vtkActor CompareActor
    CompareActor SetMapper CompareMapper
    CompareActor SetProperty property
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
    FEdgesMapper SetScalarModeToUseCellData
vtkActor FEdgesActor
    FEdgesActor SetMapper FEdgesMapper

Renderer ResetCamera
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
	.mbar.edit.menu entryconfigure 4 -state disabled
	.mbar.edit.menu entryconfigure 5 -state disabled
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
            .mbar.edit.menu entryconfigure 6 -state normal
            set s [format "Vertices:%d    Cells:%d" \
                   $NumberOfNodes $NumberOfElements]
	} else {
            .mbar.edit.menu entryconfigure 2 -state normal
            .mbar.edit.menu entryconfigure 6 -state disabled
            set s [format "Vertices:%d    Triangles:%d" \
                   $NumberOfNodes $NumberOfElements]
	}
	.mbar.edit.menu entryconfigure 3 -state normal
	.mbar.edit.menu entryconfigure 4 -state normal
	.mbar.edit.menu entryconfigure 5 -state normal
	.mbar.file.menu entryconfigure 2 -state normal
	.mbar.options.menu entryconfigure 1 -state normal
    }

    .bottomF.status configure -text $s
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

proc UpdateSmoothGUI {} {

   .smooth.f1.num set [smooth GetNumberOfIterations]
   .smooth.f1.fact set [smooth GetRelaxationFactor]
}

proc ApplySmooth {} {
    global RenWin

    smooth SetInput PolyData    

    smooth SetNumberOfIterations [.smooth.f1.num get]
    smooth SetRelaxationFactor [.smooth.f1.fact get]

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

proc UpdateCleanGUI {} {
    .clean.f1.s set [cleaner GetTolerance]
}

proc ApplyClean {} {
    global RenWin

    cleaner SetInput PolyData
    cleaner SetTolerance [.clean.f1.s get]

    UpdateUndo "cleaner"
    UpdateGUI

    $RenWin Render
    CloseClean
}
########################## The connectivity GUI
#
# Procedure defines GUI and behavior for extracting connected data. Connecting
# means extracting all cells joined at a vertex.

proc Connect {} {
    UpdateConnectGUI
    wm deiconify .connect
}
proc CloseConnect {} {
    wm withdraw .connect
}

toplevel .connect
wm withdraw .connect
wm title .connect "Extract Connected Data"
wm protocol .connect WM_DELETE_WINDOW {wm withdraw .connect}

frame .connect.fb
button .connect.fb.apply -text Apply -command ApplyConnect
button .connect.fb.cancel -text Cancel -command CloseConnect
pack .connect.fb.apply .connect.fb.cancel -side left -expand 1 -fill x
pack .connect.fb -side top -fill both -expand 1

proc UpdateConnectGUI {} {
}

proc ApplyConnect {} {
    global RenWin

    connect SetInput PolyData

    UpdateUndo "connect"
    UpdateGUI

    $RenWin Render
    CloseConnect
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

proc UpdateTriGUI {} {
}

proc ApplyTri {} {
    global RenWin

    tri SetInput PolyData

    UpdateUndo "tri"
    UpdateGUI

    $RenWin Render
    CloseTri
}

########################## The surface normals GUI
#
# Procedure defines GUI and behavior for generating surface normals. This will
# convert all polygons into triangles.
proc Normals {} {
    UpdateNormalsGUI
    wm deiconify .normals
}
proc CloseNormals {} {
    wm withdraw .normals
}

toplevel .normals
wm withdraw .normals
wm title .normals "Generate Surface Normals"
wm protocol .normals WM_DELETE_WINDOW {wm withdraw .normals}

frame .normals.f1
scale .normals.f1.fangle -label "Feature Angle" \
	-from 0 -to 180 -length 3.0i -orient horizontal -resolution 1
checkbutton .normals.f1.split -variable edgeSplitting \
	-text "Edge Splitting"
checkbutton .normals.f1.flip -variable flipNormals \
	-text "Flip Normals"
pack .normals.f1.fangle .normals.f1.split .normals.f1.flip \
	-pady 0.1i -side top -anchor w

frame .normals.fb
button .normals.fb.apply -text Apply -command ApplyNormals
button .normals.fb.cancel -text Cancel -command CloseNormals
pack .normals.fb.apply .normals.fb.cancel -side left -expand 1 -fill x
pack .normals.f1 .normals.fb -side top -fill both -expand 1

proc UpdateNormalsGUI {} {
    .normals.f1.fangle set [normals GetFeatureAngle]
}

proc ApplyNormals {} {
    global edgeSplitting flipNormals
    global RenWin

    normals SetFeatureAngle [.normals.f1.fangle get]
    normals SetSplitting $edgeSplitting
    normals SetFlipNormals $flipNormals

    normals SetInput PolyData

    UpdateUndo "normals"
    UpdateGUI

    $RenWin Render
    CloseNormals
}

############################ Setting background color
##
proc BackgroundColor {} {

    set background [Renderer GetBackground]
    .back.f1.l.r set [expr [lindex $background 0] * 255.0]
    .back.f1.l.g set [expr [lindex $background 1] * 255.0]
    .back.f1.l.b set [expr [lindex $background 2] * 255.0]
    wm deiconify .back
}

proc CloseBackground {} {
    wm withdraw .back
}

toplevel .back
wm withdraw .back
wm title .back "Select Background Color"
wm protocol .back WM_DELETE_WINDOW {wm withdraw .back}
frame .back.f1

frame .back.f1.l  -relief raised -borderwidth 3
scale .back.f1.l.r -from 255 -to 0 -orient vertical -background #f00 \
	-command SetColor
scale .back.f1.l.g -from 255 -to 0 -orient vertical -background #0f0 \
	-command SetColor
scale .back.f1.l.b -from 255 -to 0 -orient vertical -background #00f \
	-command SetColor
pack .back.f1.l.r .back.f1.l.g .back.f1.l.b -side left -fill both

frame .back.f1.m -relief raised -borderwidth 3
label .back.f1.m.sample -highlightthickness 0 -text "  Background Color  "
pack .back.f1.m.sample -fill both -expand 1

frame .back.f1.r -relief raised -borderwidth 3
image create photo ColorWheel -file ColorWheel.ppm
label .back.f1.r.wheel -image ColorWheel -highlightthickness 0
bind .back.f1.r.wheel <Button-1> {
    scan [ColorWheel get %x %y] "%%f %%f %%f" r g b
    .back.f1.l.r set $r
    .back.f1.l.g set $g
    .back.f1.l.b set $b
}
pack .back.f1.r.wheel -fill both
pack .back.f1.l .back.f1.m .back.f1.r -side left -expand 1 -fill both

frame .back.fb
button .back.fb.apply -text Apply -command ApplyBackground
button .back.fb.cancel -text Cancel -command CloseBackground
pack .back.fb.apply .back.fb.cancel -side left -expand 1 -fill x
pack .back.f1 .back.fb -side top -fill both -expand 1

proc SetColor {value} {
    set color [format #%02x%02x%02x [.back.f1.l.r get] [.back.f1.l.g get]\
	    [.back.f1.l.b get]]
    .back.f1.m.sample config -background $color
}

proc ApplyBackground {} {
    global RenWin

    Renderer SetBackground [expr [.back.f1.l.r get]/255.0] \
	    [expr [.back.f1.l.g get]/255.0] \
	    [expr [.back.f1.l.b get]/255.0]
    CompareRenderer SetBackground [expr [.back.f1.l.r get]/255.0] \
	    [expr [.back.f1.l.g get]/255.0] \
	    [expr [.back.f1.l.b get]/255.0]
    Render $RenWin
}

############################ Set surface properties
##
proc Properties {} {

    set color [property GetColor]
    .prop.f1.l.r set [expr [lindex $color 0] * 255.0]
    .prop.f1.l.g set [expr [lindex $color 1] * 255.0]
    .prop.f1.l.b set [expr [lindex $color 2] * 255.0]
    .prop.sliders.amb set [property GetAmbient]
    .prop.sliders.diff set [property GetDiffuse]
    .prop.sliders.spec set [property GetSpecular]
    .prop.sliders.power set [property GetSpecularPower]

    wm deiconify .prop
}

proc CloseProperties {} {
    wm withdraw .prop
}

toplevel .prop
wm withdraw .prop
wm title .prop "Set Surface Properties"
wm protocol .prop WM_DELETE_WINDOW {wm withdraw .prop}
frame .prop.f1

frame .prop.f1.l  -relief raised -borderwidth 3
scale .prop.f1.l.r -from 255 -to 0 -orient vertical -background #f00 \
	-command SetSurfaceColor
scale .prop.f1.l.g -from 255 -to 0 -orient vertical -background #0f0 \
	-command SetSurfaceColor
scale .prop.f1.l.b -from 255 -to 0 -orient vertical -background #00f \
	-command SetSurfaceColor
pack .prop.f1.l.r .prop.f1.l.g .prop.f1.l.b -side left -fill both

frame .prop.f1.m -relief raised -borderwidth 3
label .prop.f1.m.sample -highlightthickness 0 -text "   Surface Color    "
pack .prop.f1.m.sample -fill both -expand 1

frame .prop.f1.r -relief raised -borderwidth 3
image create photo ColorWheel -file ColorWheel.ppm
label .prop.f1.r.wheel -image ColorWheel -highlightthickness 0
bind .prop.f1.r.wheel <Button-1> {
    scan [ColorWheel get %x %y] "%%f %%f %%f" r g b
    .prop.f1.l.r set $r
    .prop.f1.l.g set $g
    .prop.f1.l.b set $b
}
pack .prop.f1.r.wheel -fill both
pack .prop.f1.l .prop.f1.m .prop.f1.r -side left -expand 1 -fill both

frame .prop.sliders
scale .prop.sliders.amb -from 0.00 -to 1.00 -orient horizontal\
	-resolution 0.01 -label Ambient
scale .prop.sliders.diff -from 0.00 -to 1.00 -orient horizontal\
	-resolution 0.01 -label Diffuse
scale .prop.sliders.spec -from 0.00 -to 1.00 -orient horizontal\
	-resolution 0.01 -label Specular
scale .prop.sliders.power -from 0 -to 100 -orient horizontal\
	-resolution 1 -label "Specular Power"
pack .prop.sliders.spec .prop.sliders.power .prop.sliders.amb\
	.prop.sliders.diff -side top -fill both

frame .prop.fb
button .prop.fb.apply -text Apply -command ApplyProperties
button .prop.fb.cancel -text Cancel -command CloseProperties
pack .prop.fb.apply .prop.fb.cancel -side left -expand 1 -fill x
pack .prop.f1 .prop.sliders .prop.fb -side top -fill both -expand 1

proc SetSurfaceColor {value} {
    set color [format #%02x%02x%02x [.prop.f1.l.r get] [.prop.f1.l.g get]\
	    [.prop.f1.l.b get]]
    .prop.f1.m.sample config -background $color
}

proc ApplyProperties {} {
    global RenWin

    property SetColor [expr [.prop.f1.l.r get]/255.0] \
	    [expr [.prop.f1.l.g get]/255.0] \
	    [expr [.prop.f1.l.b get]/255.0]
    property SetAmbient [.prop.sliders.amb get]
    property SetDiffuse [.prop.sliders.diff get]
    property SetSpecular [.prop.sliders.spec get]
    property SetSpecularPower [.prop.sliders.power get]
    Render $RenWin
}

#------------------------Procedures for ProgressWidget----------------------
proc StartProgress {filter label} {
   global BarId
   global TextId

   set height [winfo height .bottomF.status]
   set width [winfo width .bottomF.status]

   if { ![winfo exists .bottomF.canvas] } {
      canvas .bottomF.canvas -height $height -width $width -borderwidth 0\
	    -highlightthickness 0
   } else {
      .bottomF.canvas configure -height $height -width $width
      .bottomF.canvas delete $BarId
      .bottomF.canvas delete $TextId
   }

   set BarId [.bottomF.canvas create rect 0 0 0 $height -fill #888]
   set TextId [.bottomF.canvas create text [expr $width/2] [expr $height/2] \
	   -anchor center -justify center -text $label]
   pack forget .bottomF.status
   pack .bottomF.canvas -padx 0 -pady 0

   update
}

proc ShowProgress {filter label} {
   global BarId
   global TextId

   set progress [$filter GetProgress]
   set height [winfo height .bottomF.status]
   set width [winfo width .bottomF.status]

   .bottomF.canvas delete $BarId
   .bottomF.canvas delete $TextId
   set BarId [.bottomF.canvas create rect 0 0 [expr $progress*$width] $height \
	 -fill #888]
   set TextId [.bottomF.canvas create text [expr $width/2] [expr $height/2] \
	   -anchor center -justify center -text $label]

   update
}

proc EndProgress {} {
   pack forget .bottomF.canvas
   pack .bottomF.status -side top -anchor w -expand 1 -fill x

   update
}

