# Load the vtk tcl library
catch {load vtktcl}

# Source the interactor that we will use for the TkRenderWidget
source TkInteractor.tcl

# Set some variables to default values
set low_res 4
set med_res 12
set high_res 24

# Remove the default toplevel
wm withdraw .

# Create the toplevel window
toplevel .top
wm title .top {Sphere Flake Frustum Coverage Culling}

# Create some frames
frame .top.f1 
frame .top.f2
pack .top.f1 .top.f2 -side left -expand 1 -fill both

# Create the TkRenderWidget
vtkTkRenderWidget .top.f1.rw -width 500 -height 500
BindTkRenderWidget .top.f1.rw
pack .top.f1.rw -expand 1 -fill both

# Create the renderer and add it to the render window
vtkRenderer ren1
set renWin [.top.f1.rw GetRenderWindow]
$renWin AddRenderer ren1

# Create the initial flake list - it has one sphere at
# (0.0, 0.0, 0.0) with a radius of 1.0 and a color of
# (0.8, 0.2, 0.8)
set flake_list ""
set sphere [list 0.0 0.0 0.0 1.0 0.8 0.2 0.8]
set flake_list [linsert $flake_list end $sphere]

# Create a transform that will be used in the AddSpheres proc
vtkTransform t

# Take the flake list, and for each sphere of the specified
# radius, add nine more spheres around it
proc AddSpheres { flake_list radius } {

    set new_list ""

    foreach s $flake_list {
      set x [lindex $s 0]
      set y [lindex $s 1]
      set z [lindex $s 2]
      set r [lindex $s 3]

      set new_list [linsert $new_list end $s]

      if { $r == $radius } {
	
	set x_angle 80
	set y_angle 0

	set new_r [expr $r / 5.0]

	for { set i 0 } { $i < 9 } { incr i } {
	    t Identity
	    t RotateX $x_angle
	    t RotateY $y_angle

	    t SetPoint 0 0 1 1
	    set point [t GetPoint]

	    set new_x [expr ( $x + [lindex $point 0] * ( $r * 1.5 ) )]
	    set new_y [expr ( $y + [lindex $point 1] * ( $r * 1.5 ) )]
	    set new_z [expr ( $z + [lindex $point 2] * ( $r * 1.5 ) )]

	    switch $i {
		0 {set cr 1.0; set cg 0.0; set cb 0.0}
		1 {set cr 1.0; set cg 0.4; set cb 0.0}
		2 {set cr 1.0; set cg 1.0; set cb 0.2}
		3 {set cr 0.5; set cg 1.0; set cb 0.4}
		4 {set cr 0.0; set cg 0.8; set cb 0.0}
		5 {set cr 0.0; set cg 0.8; set cb 1.0}
		6 {set cr 0.0; set cg 0.0; set cb 1.0}
		7 {set cr 0.5; set cg 0.0; set cb 1.0}
		8 {set cr 1.0; set cg 0.0; set cb 1.0}
	    }
	    
	    set new_list [linsert $new_list end [list $new_x $new_y $new_z $new_r $cr $cg $cb]]
	    set x_angle [expr $x_angle - 20.0]
	    set y_angle [expr $y_angle + 70.0]
	}
      }
    }
    return $new_list
}

# Take the initial sphere list and add spheres recursively
set r 1.0
for { set i 0 } { $i < 3 } { incr i } {
    set flake_list [AddSpheres $flake_list $r]

    set r [expr $r / 5.0]
}


## Create the high resolution sphere source and mapper
vtkSphereSource sphere
sphere SetCenter 0.0 0.0 0.0
sphere SetRadius 1.0
sphere SetThetaResolution $high_res
sphere SetPhiResolution $high_res
vtkPolyDataMapper mapper
mapper SetInput [sphere GetOutput]

## Create the medium resolution sphere source and mapper
vtkSphereSource med_res_sphere
med_res_sphere SetCenter 0.0 0.0 0.0
med_res_sphere SetRadius 1.0
med_res_sphere SetThetaResolution $med_res
med_res_sphere SetPhiResolution $med_res
vtkPolyDataMapper med_res_mapper
med_res_mapper SetInput [med_res_sphere GetOutput]

## Create the low resolution sphere source and mapper
vtkSphereSource low_res_sphere
low_res_sphere SetCenter 0.0 0.0 0.0
low_res_sphere SetRadius 1.0
low_res_sphere SetThetaResolution $low_res
low_res_sphere SetPhiResolution $low_res
vtkPolyDataMapper low_res_mapper
low_res_mapper SetInput [low_res_sphere GetOutput]


## Add all the actors - one for each sphere in the flake list
set i 0
foreach s $flake_list {
    vtkLODActor actor_$i
    actor_$i SetMapper mapper
    actor_$i BuildLODsOff
    actor_$i AddMapper low_res_mapper
    actor_$i AddMapper med_res_mapper
    actor_$i SetPosition [lindex $s 0] [lindex $s 1] [lindex $s 2]
    actor_$i SetScale [lindex $s 3] [lindex $s 3] [lindex $s 3]

    set r [lindex $s 4]
    set g [lindex $s 5]
    set b [lindex $s 6]

    [actor_$i GetProperty] SetColor $r $g $b

    ren1 AddActor actor_$i

    incr i
}

## Add the frustum coverage culler to the renderer
vtkFrustumCoverageCuller culler
ren1 AddCuller culler


## Create some UI stuff for controling things

frame .top.f2.f1 -bg #6600ff -bd 2 -relief groove
frame .top.f2.f2 -bg #6600ff -bd 2 -relief groove
frame .top.f2.f3 -bg #6600ff -bd 2 -relief groove

pack .top.f2.f1 .top.f2.f2 .top.f2.f3 \
	-side top -expand 1 -fill both

scale .top.f2.f1.s1 -label " Low Res Sphere: " -orient horizontal \
	-length 200 -from 3 -to 8 -variable low_res \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -troughcolor #334433
scale .top.f2.f1.s2 -label " Med Res Sphere: " -orient horizontal \
	-length 200 -from 9 -to 15 -variable med_res \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -troughcolor #334433
scale .top.f2.f1.s3 -label "High Res Sphere: " -orient horizontal \
	-length 200 -from 16 -to 30 -variable high_res \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -troughcolor #334433

bind .top.f2.f1.s1 <ButtonRelease> { 
    low_res_sphere SetThetaResolution $low_res
    low_res_sphere SetPhiResolution $low_res
    $renWin Render
}

bind .top.f2.f1.s2 <ButtonRelease> { 
    med_res_sphere SetThetaResolution $med_res
    med_res_sphere SetPhiResolution $med_res
    $renWin Render
}

bind .top.f2.f1.s3 <ButtonRelease> { 
    sphere SetThetaResolution $high_res
    sphere SetPhiResolution $high_res
    $renWin Render
}

pack .top.f2.f1.s1 .top.f2.f1.s2 .top.f2.f1.s3 -side top -expand 1 -fill both


set min_coverage [culler GetMinimumCoverage]
set max_coverage [culler GetMaximumCoverage]

scale .top.f2.f2.s1 -label "Minumum Coverage: " -orient horizontal \
	-length 200 -from 0.0000 -to 0.0010 -variable min_coverage \
	-resolution 0.00001 \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -troughcolor #334433
scale .top.f2.f2.s2 -label "Maximum Coverage: " -orient horizontal \
	-length 200 -from 0.01 -to 1.0 -variable max_coverage \
	-resolution 0.01 \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -troughcolor #334433


bind .top.f2.f2.s1 <ButtonRelease> { 
    culler SetMinimumCoverage $min_coverage
    $renWin Render
}

bind .top.f2.f2.s2 <ButtonRelease> { 
    culler SetMaximumCoverage $max_coverage
    $renWin Render
}

pack .top.f2.f2.s1 .top.f2.f2.s2 -side top -expand 1 -fill both


button .top.f2.f3.b1 -text "Quit" -command {exit} \
	-bg #000000 -fg #ccffcc \
	-activebackground #000000 -activeforeground #55ff55 -bd 3

pack .top.f2.f3.b1  -expand 1 -fill both