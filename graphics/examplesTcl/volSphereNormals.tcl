catch {load vtktcl}

source TkInteractor.tcl

set recursion_depth 3

set triangle_count 0
set point_count 0

proc AddNextLevel { in_vl } {

  set out_vl {}

  foreach t $in_vl {
    set p1 [lindex $t 0]
    set p2 [lindex $t 1]
    set p3 [lindex $t 2]

    set p1x [lindex $p1 0]
    set p1y [lindex $p1 1]
    set p1z [lindex $p1 2]

    set p2x [lindex $p2 0]
    set p2y [lindex $p2 1]
    set p2z [lindex $p2 2]

    set p3x [lindex $p3 0]
    set p3y [lindex $p3 1]
    set p3z [lindex $p3 2]

    set p12x [expr ($p1x + $p2x) / 2.0]
    set p12y [expr ($p1y + $p2y) / 2.0]
    set p12z [expr ($p1z + $p2z) / 2.0]

    set p23x [expr ($p2x + $p3x) / 2.0]
    set p23y [expr ($p2y + $p3y) / 2.0]
    set p23z [expr ($p2z + $p3z) / 2.0]

    set p31x [expr ($p3x + $p1x) / 2.0]
    set p31y [expr ($p3y + $p1y) / 2.0]
    set p31z [expr ($p3z + $p1z) / 2.0]

    set norm [expr sqrt ([expr (($p12x * $p12x) + ($p12y * $p12y) + ($p12z * $p12z)) ])]
    set p12x [expr $p12x / $norm]
    set p12y [expr $p12y / $norm]
    set p12z [expr $p12z / $norm]

    set norm [expr sqrt ([expr (($p23x * $p23x) + ($p23y * $p23y) + ($p23z * $p23z)) ])]
    set p23x [expr $p23x / $norm]
    set p23y [expr $p23y / $norm]
    set p23z [expr $p23z / $norm]

    set norm [expr sqrt ([expr (($p31x * $p31x) + ($p31y * $p31y) + ($p31z * $p31z)) ])]
    set p31x [expr $p31x / $norm]
    set p31y [expr $p31y / $norm]
    set p31z [expr $p31z / $norm]

    set out_vl [lappend out_vl \
      [subst {{$p1x $p1y $p1z} {$p12x $p12y $p12z} {$p31x $p31y $p31z}} ]]

    set out_vl [lappend out_vl \
      [subst {{$p2x $p2y $p2z} {$p12x $p12y $p12z} {$p23x $p23y $p23z}} ]]

    set out_vl [lappend out_vl \
      [subst {{$p3x $p3y $p3z} {$p23x $p23y $p23z} {$p31x $p31y $p31z}} ]]

    set out_vl [lappend out_vl \
      [subst {{$p12x $p12y $p12z} {$p23x $p23y $p23z} {$p31x $p31y $p31z}} ]]
    }

  return $out_vl
}

proc AddPoints { vl } {
    global point_count

    foreach t $vl {
	foreach v $t {
	    points InsertPoint $point_count [lindex $v 0] [lindex $v 1] [lindex $v 2]
	    incr point_count
	}
    }
}

proc AddTriangles { vl } {
    global triangle_count

    set num_tris [llength $vl]
    
    for { set i 0 } { $i < $num_tris } { incr i; incr triangle_count } {
	triangles InsertNextCell 4
	triangles InsertCellPoint [expr $triangle_count * 3]
	triangles InsertCellPoint [expr $triangle_count * 3 + 1]
	triangles InsertCellPoint [expr $triangle_count * 3 + 2]
	triangles InsertCellPoint [expr $triangle_count * 3]
    }
}

proc MakeSphere { } {
    global recursion_depth
    global triangle_count
    global point_count

    points SetNumberOfPoints 0
    triangles Reset

    set triangle_count 0
    set point_count 0

    set initial_list {}

    set initial_list [lappend initial_list \
	    { { 0 1 0 } { 1 0 0 } { 0 0 1 } } ]
    set initial_list [lappend initial_list \
	    { { 0 1 0 } { 0 0 1 } { -1 0 0 } } ]
    set initial_list [lappend initial_list \
	    { { 0 1 0 } { -1 0 0 } { 0 0 -1 } } ]
    set initial_list [lappend initial_list \
	    { { 0 1 0 } { 0 0 -1 } { 1 0 0 } } ]
    
    set initial_list [lappend initial_list \
	    { { 0 -1 0 } { 1 0 0 } { 0 0 1 } } ]
    set initial_list [lappend initial_list \
	    { { 0 -1 0 } { 0 0 1 } { -1 0 0 } } ]
    set initial_list [lappend initial_list \
	    { { 0 -1 0 } { -1 0 0 } { 0 0 -1 } } ]
    set initial_list [lappend initial_list \
	    { { 0 -1 0 } { 0 0 -1 } { 1 0 0 } } ]

    if { $recursion_depth < 2 } {
	set vertex_list $initial_list

	for { set i 0 } { $i < $recursion_depth } { incr i } {
	    set vertex_list [AddNextLevel $vertex_list]
	}
	
	AddPoints $vertex_list 
	AddTriangles $vertex_list
    } else {
	foreach t1 $initial_list {
	    set tmp_list [AddNextLevel [list $t1]]
	    foreach t2 $tmp_list {
		set tmp_list2 [AddNextLevel [list $t2]]
		foreach t3 $tmp_list2 {
		    set vertex_list [list $t3]
		    for { set i 0 } { $i < [expr $recursion_depth - 2] } { incr i } {
			set vertex_list [AddNextLevel $vertex_list]
		    }
		    AddPoints $vertex_list
		    AddTriangles $vertex_list
		}
	    }
	}
    }

  sphereMapper Modified
  spherePolyData Modified
}

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/sphere.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  100  0.0
    opacityTransferFunction AddPoint  128  1.0

vtkPiecewiseFunction colorTransferFunction
    colorTransferFunction AddPoint      0.0 1.0
    colorTransferFunction AddPoint    255.0 1.0

vtkPiecewiseFunction gradtf
  gradtf AddPoint 0.0 0.0
  gradtf AddPoint 1.0 1.0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOn
    volumeProperty SetInterpolationTypeToLinear


vtkRecursiveSphereDirectionEncoder directionEncoder
    directionEncoder SetRecursionDepth $recursion_depth

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetScalarInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction
    [volumeMapper GetGradientEstimator] SetDirectionEncoder directionEncoder

vtkVolume volume
    volume SetVolumeMapper volumeMapper
    volume SetVolumeProperty volumeProperty

# Create outline
vtkOutlineFilter outline
    outline SetInput [reader GetOutput]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 1 1 1


vtkFloatPoints points
vtkCellArray   triangles

vtkPolyData spherePolyData
  spherePolyData SetPoints points
  spherePolyData SetPolys triangles

vtkCleanPolyData cleaner
  cleaner SetInput spherePolyData

vtkPolyDataNormals normals
  normals SetInput [cleaner GetOutput]
  normals FlipNormalsOn

vtkPolyDataMapper sphereMapper
  sphereMapper SetInput [normals GetOutput]

vtkActor sphereActor 
  sphereActor SetMapper sphereMapper
  [sphereActor GetProperty] SetColor 1.0 1.0 1.0
  [sphereActor GetProperty] SetAmbient 0.3
  [sphereActor GetProperty] SetDiffuse 0.7
  [sphereActor GetProperty] SetRepresentationToWireframe

MakeSphere

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

ren1 AddActor outlineActor
ren1 AddVolume volume
ren1 SetBackground 0.1 0.2 0.4

vtkRenderer ren2
vtkRenderWindow renWin2
   renWin2 AddRenderer ren2

ren2 AddActor sphereActor
ren2 SetBackground 0.1 0.2 0.4

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

proc pow { a b } {

    set c 1

    for { set i 0 } { $i < $b } { incr i } {
	set c [expr $c * $a]
    }

    return $c
}

proc SetInfoLabel { } {
    global recursion_depth

    set dirs [directionEncoder GetNumberOfEncodedDirections]
    set outer_size [expr [pow 2 $recursion_depth] + 1]
    set unique [expr $dirs - 1 - ($outer_size * 4) + 4]

    .top.f3.info config -text \
"At a recursion depth of $recursion_depth there
are $dirs encoded directions. This is 
$unique unique directions, [expr $dirs - $unique - 1] 
duplicated directions on the z=0 plane, 
and 1 encoded index for the zero normal.
"
}

wm withdraw .

toplevel .top
wm title .top {Recursive Sphere Normal Encoding}

frame .top.f1 -bd 0 -bg #193264
frame .top.f2 -bd 0 -bg #193264
frame .top.f3 -bd 0 -bg #193264
pack .top.f1 .top.f2 .top.f3 -side left -expand 1 -fill both

vtkTkRenderWidget .top.f1.rw -width 200 -height 200 -rw renWin
BindTkRenderWidget .top.f1.rw
pack .top.f1.rw -side top -expand 1 -fill both

label .top.f1.info -bg #193264 -fg #aaaaaa -text \
"A 50x50x50 volume created 
from the sphere distance 
function, rendered with 
trilinear interpolation,
shading, and scalar opacity 
compositing."
pack .top.f1.info -side top -expand 1 -fill both

vtkTkRenderWidget .top.f2.rw -width 200 -height 200 -rw renWin2
BindTkRenderWidget .top.f2.rw
pack .top.f2.rw -side top -expand 1 -fill both

label .top.f2.info -bg #193264 -fg #aaaaaa -text \
"This wireframe sphere
represents the direction
encoding. Each vertex has
an index. A direction is
encoded into the index of
the closest vertex."

pack .top.f2.info -side top -expand 1 -fill both

label .top.f3.info -bg #193264 -fg #aaaaaa -text "hello world" \
	-bd 0 -justify left 

pack .top.f3.info -side top -expand 0 -fill both -padx 10 -pady 10
SetInfoLabel

scale .top.f3.level -label "Recursion Depth" -orient horizontal \
	-length 200 -from 0 -to 6 -variable recursion_depth \
	-bg #193264 -fg #aaaaaa -bd 0 -highlightthickness 0 \
	-troughcolor #777777 -activebackground #385284 


pack .top.f3.level -side top -expand 0 -fill both -padx 10 -pady 10

label .top.f3.working  -bg #193264 -fg #ff3333 -text "" -justify center
pack .top.f3.working -side top -expand 0 -fill both -padx 10 -pady 10

label .top.f3.timeinfo  -bg #193264 -fg #ff3333 -text "" -justify center
pack .top.f3.timeinfo -side top -expand 0 -fill both -padx 10 -pady 10


bind .top.f3.level <ButtonRelease> { 
    global working
    global recursion_depth
    
    .top.f3.working configure -text "Working"
    .top.f3.timeinfo configure -text \
"(The wireframe sphere is generated 
in tcl, therefore this may take a 
few minutes at the highest levels.)"
    set working 1
    update

    directionEncoder SetRecursionDepth $recursion_depth
    MakeSphere
    SetInfoLabel
    renWin Render
    renWin2 Render

    .top.f3.working configure -text ""
    .top.f3.timeinfo configure -text ""
    update
}

#renWin SetFileName "valid/volSphereNormals.tcl.ppm"
#renWin SaveImageAsPPM



















