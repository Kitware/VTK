catch {load vtktcl}

source volTkInteractor.tcl

set demo_font "-*-helvetica-bold-r-normal--17-*-*-*-p-92-iso8859-1"
### Create the top level stuff and basic frames

wm withdraw .

toplevel .top -visual best

frame .top.f1
frame .top.f2 -bg #999999

frame .top.f1.f1 -bd 4 -bg #770099 -relief ridge
frame .top.f1.f2

pack .top.f1 .top.f2 -side top -expand 1 -fill both

pack .top.f1.f1 .top.f1.f2 -side left -expand 1 -fill both



### Create the render windows

frame .top.f1.f2.f1 -bd 0
frame .top.f1.f2.f2 -bd 0

frame .top.f1.f2.f1.f1 -bd 4 -bg #770099 -relief ridge 

frame .top.f1.f2.f2.f1 -bd 4 -bg #770099 -relief ridge 

label .top.f1.f2.f1.f1.label -text "Slice Viewer" \
	-bg #999999 -fg #440066 \
	-font $demo_font

label .top.f1.f2.f2.f1.label -text "Composite Rendering" \
	-bg #999999 -fg #440066 \
	-font $demo_font

vtkTkRenderWidget .top.f1.f2.f1.f1.ren -width 256 -height 256 
    BindTkRenderWidget .top.f1.f2.f1.f1.ren

vtkTkRenderWidget .top.f1.f2.f2.f1.ren -width 256 -height 256 
    BindTkRenderWidget .top.f1.f2.f2.f1.ren

pack .top.f1.f2.f1 .top.f1.f2.f2 -side top

pack .top.f1.f2.f1.f1 -side left
pack .top.f1.f2.f2.f1 -side left

pack .top.f1.f2.f1.f1.label .top.f1.f2.f1.f1.ren -side top -expand 1 -fill both
pack .top.f1.f2.f2.f1.label .top.f1.f2.f2.f1.ren -side top -expand 1 -fill both

set renWin1 [.top.f1.f2.f1.f1.ren GetRenderWindow]
set ren1   [$renWin1 MakeRenderer]

set renWin3 [.top.f1.f2.f2.f1.ren GetRenderWindow]
set ren3   [$renWin3 MakeRenderer]

$renWin1 SetSize 256 256
$renWin3 SetSize 256 256


### Create the help message area

label .top.f2.help_label -text "VTk Volume Rendering Example" \
	-bg #999999 -fg #440066 \
	-font $demo_font

label .top.f2.help_message \
	-bg #999999 -fg #000000 \
	-font $demo_font \
	-text \
"Left Mouse Button: rotate
Shift-Left or Middle Mouse Button: pan
Right Mouse Button: zoom
r key: reset view"

button .top.f2.exit -text Exit -fg #999999 -bg #440066 \
	-activeforeground #440066 -activebackground #999999 \
	-highlightthickness 0 -bd 4 -command { exit } \
	-font $demo_font

pack .top.f2.help_label .top.f2.help_message .top.f2.exit -padx 5 -pady 5 \
	-expand 1 -fill both


### Create the control options area

frame .top.f1.f1.f1 -bg #999999

label .top.f1.f1.f1.label -text "Control Options" \
	-bg #999999 -fg #000000 \
	-font $demo_font

set control_menu_value {Slice Viewer Controls}

set control_menu [tk_optionMenu .top.f1.f1.f1.menu control_menu_value \
        {Slice Viewer Controls} \
	{Composite Controls} ]

.top.f1.f1.f1.menu configure -fg #999999 -bg #440066 \
	-activeforeground #440066 -activebackground #999999 \
	-highlightthickness 0 -bd 4 \
	-font $demo_font

$control_menu configure -bg #999999 -fg #440066 \
	-activeforeground #220033 -activebackground #bbbbbb \
	-font $demo_font

bind $control_menu <Unmap> { after 100 { update ; change_active_controls } }


pack .top.f1.f1.f1 -side top -expand 1 -fill both 
pack .top.f1.f1.f1.label  -side top -expand 0 -fill both -padx 50 -pady 5
pack .top.f1.f1.f1.menu -side top -expand 0 -fill none -padx 2 -pady 5


### Create the control options area for Composite

set ww .top.f1.f1.f1

frame $ww.comp -bg #999999

label $ww.comp.l1 -text "Interpolation Type:" -bg #999999 -fg #000000 \
	-font $demo_font

set comp_interpolation_type 0

radiobutton $ww.comp.rb1 -text "Nearest Neighbor" \
	-variable comp_interpolation_type -value 0 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop SetInterpolationTypeToNearest ; $renWin3 Render }


radiobutton $ww.comp.rb2 -text "Trilinear" \
	-variable comp_interpolation_type -value 1 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop SetInterpolationTypeToLinear ; $renWin3 Render }
 
pack $ww.comp.l1 -side top -padx 2 -pady 2 -expand 0 -fill none -anchor w

pack $ww.comp.rb1 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w
pack $ww.comp.rb2 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w

label $ww.comp.l2 -text "Value To Color Mapping:" -bg #999999 -fg #000000 \
	-font $demo_font \

set comp_color_type 1

radiobutton $ww.comp.rb3 -text "Greyscale" \
	-variable comp_color_type -value 0 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop SetColor gtfun; $renWin3 Render }


radiobutton $ww.comp.rb4 -text "Color" \
	-variable comp_color_type -value 1 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop SetColor ctfun; $renWin3 Render }

 
pack $ww.comp.l2 -side top -padx 2 -pady 2 -expand 0 -fill none -anchor w

pack $ww.comp.rb3 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w
pack $ww.comp.rb4 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w

label $ww.comp.l3 -text "Shading:" -bg #999999 -fg #000000 \
	-font $demo_font

set comp_shade_type 0

radiobutton $ww.comp.rb5 -text "Off" \
	-variable comp_shade_type -value 0 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop ShadeOff ; tfun AddPoint 255.0 0.2 ; $renWin3 Render }


radiobutton $ww.comp.rb6 -text "On" \
	-variable comp_shade_type -value 1 \
	-bg #999999 -fg #000000 -selectcolor #aa00ff \
	-highlightthickness 0 -activebackground #999999 \
	-font $demo_font \
	-activeforeground #7700ff -command \
	{ comp_prop ShadeOn ; tfun AddPoint 255.0 0.4 ; $renWin3 Render }
 
 
pack $ww.comp.l3 -side top -padx 2 -pady 2 -expand 0 -fill none -anchor w

pack $ww.comp.rb5 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w
pack $ww.comp.rb6 -side top -padx 22 -pady 2 -expand 0 -fill none -anchor w



vtkSLCReader reader
    reader SetFileName "../../../data/poship.slc"

reader Update

set bounds [[reader GetOutput] GetBounds]
scan $bounds "%f %f %f %f %f %f" minx maxx miny maxy minz maxz

set xdiff [expr $maxx - $minx];
set ydiff [expr $maxy - $miny];
set zdiff [expr $maxz - $minz];


### Create the control options area for the slice viewer

set ww .top.f1.f1.f1

frame $ww.slice -bg #999999

frame $ww.slice.x -bg #999999
frame $ww.slice.y -bg #999999
frame $ww.slice.z -bg #999999

pack $ww.slice.x $ww.slice.y $ww.slice.z -side top -padx 2 -pady 5 \
	-expand 0 -fill none

label $ww.slice.x.l1 -text "X Slice:" -bg #999999 -fg #000000

set x_slice_value [expr $xdiff / 2.0]

scale $ww.slice.x.s1 -length 100 -from $minx -to $maxx -resolution 0.5 \
	-bg #999999 -fg #770099 -variable x_slice_value \
	-orient horizontal -highlightthickness 0

pack $ww.slice.x.l1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w
pack $ww.slice.x.s1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w

bind $ww.slice.x.s1 <ButtonRelease> { set_x_slice_value }


label $ww.slice.y.l1 -text "Y Slice:" -bg #999999 -fg #000000

set y_slice_value [expr $ydiff / 2.0]

scale $ww.slice.y.s1 -length 100 -from $miny -to $maxy -resolution 0.5 \
	-bg #999999 -fg #770099 -variable y_slice_value \
	-orient horizontal -highlightthickness 0

pack $ww.slice.y.l1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w
pack $ww.slice.y.s1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w

bind $ww.slice.y.s1 <ButtonRelease> { set_y_slice_value }


label $ww.slice.z.l1 -text "Z Slice:" -bg #999999 -fg #000000

set z_slice_value [expr $zdiff / 2.0]

scale $ww.slice.z.s1 -length 100 -from $minz -to $maxz -resolution 0.5 \
	-bg #999999 -fg #770099 -variable z_slice_value \
	-orient horizontal -highlightthickness 0

pack $ww.slice.z.l1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w
pack $ww.slice.z.s1 -side left -padx 5 -pady 2 -expand 0 -fill none -anchor w

bind $ww.slice.z.s1 <ButtonRelease> { set_z_slice_value }




### Start with Slice Viewer controls up

pack .top.f1.f1.f1.slice -side top -expand 1 -fill both -pady 20

### Set up the visualization pipeline

vtkPiecewiseFunction tfun;
        tfun AddPoint  20   0.0
        tfun AddPoint  255  0.2

vtkPiecewiseFunction gtfun;
        gtfun AddPoint    0   0.0
        gtfun AddPoint  255   1.0

vtkColorTransferFunction ctfun
        ctfun AddRedPoint      0.0 0.0
        ctfun AddRedPoint     64.0 1.0
        ctfun AddRedPoint    128.0 0.0
        ctfun AddRedPoint    255.0 0.0
        ctfun AddBluePoint    0.0 0.0
        ctfun AddBluePoint   64.0 0.0
        ctfun AddBluePoint  128.0 1.0
        ctfun AddBluePoint  192.0 0.0
        ctfun AddBluePoint  255.0 0.0
        ctfun AddGreenPoint     0.0 0.0
        ctfun AddGreenPoint   128.0 0.0
        ctfun AddGreenPoint   192.0 1.0
        ctfun AddGreenPoint   255.0 0.2


vtkVolumeProperty comp_prop
        comp_prop SetColor ctfun
        comp_prop SetOpacity tfun

vtkVolumeRayCastCompositeFunction  comp_func

vtkVolumeRayCastMapper comp_volmap
        comp_volmap SetScalarInput [reader GetOutput]
        comp_volmap SetVolumeRayCastFunction comp_func

vtkVolume comp_volume
        comp_volume SetVolumeMapper comp_volmap
        comp_volume SetVolumeProperty comp_prop


vtkCubeSource outline

    outline SetXLength $xdiff;
    outline SetYLength $ydiff;
    outline SetZLength $zdiff;
    outline SetCenter  [ expr $xdiff / 2.0 ] \
                       [ expr $ydiff / 2.0 ] \
                       [ expr $zdiff / 2.0 ]

vtkPolyDataMapper outline_mapper
    outline_mapper SetInput [outline GetOutput]
    outline_mapper ImmediateModeRenderingOn

vtkActor outline_actor
    outline_actor SetMapper outline_mapper
    [outline_actor GetProperty] SetColor .7 0 .9
    [outline_actor GetProperty] SetAmbient  1
    [outline_actor GetProperty] SetDiffuse  0
    [outline_actor GetProperty] SetSpecular 0
    [outline_actor GetProperty] SetRepresentationToWireframe

### This is the color lookup table for the probe planes

vtkLookupTable ColorLookupTable
   ColorLookupTable SetHueRange 0.0 1.0 
   ColorLookupTable SetNumberOfColors 256
   ColorLookupTable SetTableRange 0 255
   ColorLookupTable SetSaturationRange 0 0
   ColorLookupTable SetValueRange 0 1
   ColorLookupTable SetAlphaRange 1 1
   ColorLookupTable Build


## This is the first probe plane

vtkPlaneSource plane1
    plane1 SetResolution 66 66

vtkTransform transform1
  transform1 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  [ expr $zdiff / 2.0 ]
  transform1 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]

vtkTransformPolyDataFilter transpd1
  transpd1 SetInput [plane1 GetOutput]
  transpd1 SetTransform transform1

vtkOutlineFilter outpd1
    outpd1 SetInput [transpd1 GetOutput]

vtkPolyDataMapper mappd1
    mappd1 SetInput [outpd1 GetOutput]

vtkActor plane_outline_actor1
    plane_outline_actor1 SetMapper mappd1
    [plane_outline_actor1 GetProperty] SetColor 0 0.5 1.0

vtkProbeFilter probe1
    probe1 SetInput [transpd1 GetOutput]
    probe1 SetSource [reader GetOutput]

vtkCastToConcrete cast1
    cast1 SetInput [probe1 GetOutput]

vtkTriangleFilter tf1
    tf1 SetInput [cast1 GetPolyDataOutput]

vtkStripper strip1
    strip1 SetInput [tf1 GetOutput]

vtkPolyDataMapper probemapper1
    probemapper1 SetInput [strip1 GetOutput]
    probemapper1 SetLookupTable ColorLookupTable
    probemapper1 SetScalarRange 0 255

vtkActor probe_actor1
    probe_actor1 SetMapper probemapper1
    [probe_actor1 GetProperty] SetOpacity 1.0

## This is the second probe plane

vtkPlaneSource plane2
    plane2 SetResolution 66 66

vtkTransform transform2
  transform2 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  [ expr $zdiff / 2.0 ]
  transform2 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]
  transform2 RotateX 90

vtkTransformPolyDataFilter transpd2
  transpd2 SetInput [plane2 GetOutput]
  transpd2 SetTransform transform2

vtkOutlineFilter outpd2
    outpd2 SetInput [transpd2 GetOutput]

vtkPolyDataMapper mappd2
    mappd2 SetInput [outpd2 GetOutput]

vtkActor plane_outline_actor2
    plane_outline_actor2 SetMapper mappd2
    [plane_outline_actor2 GetProperty] SetColor 0 0.5 1.0

vtkProbeFilter probe2
    probe2 SetInput [transpd2 GetOutput]
    probe2 SetSource [reader GetOutput]

vtkCastToConcrete cast2
    cast2 SetInput [probe2 GetOutput]

vtkTriangleFilter tf2
    tf2 SetInput [cast2 GetPolyDataOutput]

vtkStripper strip2
    strip2 SetInput [tf2 GetOutput]

vtkPolyDataMapper probemapper2
    probemapper2 SetInput [strip2 GetOutput]
    probemapper2 SetLookupTable ColorLookupTable
    probemapper2 SetScalarRange 0 255

vtkActor probe_actor2
    probe_actor2 SetMapper probemapper2
    [probe_actor2 GetProperty] SetOpacity 1.0


## This is the third probe plane

vtkPlaneSource plane3
    plane3 SetResolution 66 66

vtkTransform transform3
  transform3 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  [ expr $zdiff / 2.0 ]
  transform3 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]
  transform3 RotateY 90

vtkTransformPolyDataFilter transpd3
  transpd3 SetInput [plane3 GetOutput]
  transpd3 SetTransform transform3

vtkOutlineFilter outpd3
    outpd3 SetInput [transpd3 GetOutput]

vtkPolyDataMapper mappd3
    mappd3 SetInput [outpd3 GetOutput]

vtkActor plane_outline_actor3
    plane_outline_actor3 SetMapper mappd3
    [plane_outline_actor3 GetProperty] SetColor 0 0.5 1.0

vtkProbeFilter probe3
    probe3 SetInput [transpd3 GetOutput]
    probe3 SetSource [reader GetOutput]

vtkCastToConcrete cast3
    cast3 SetInput [probe3 GetOutput]

vtkTriangleFilter tf3
    tf3 SetInput [cast3 GetPolyDataOutput]

vtkStripper strip3
    strip3 SetInput [tf3 GetOutput]

vtkPolyDataMapper probemapper3
    probemapper3 SetInput [strip3 GetOutput]
    probemapper3 SetLookupTable ColorLookupTable
    probemapper3 SetScalarRange 0 255

vtkActor probe_actor3
    probe_actor3 SetMapper probemapper3
    [probe_actor3 GetProperty] SetOpacity 1.0



$ren1 AddActor  probe_actor1
$ren1 AddActor  probe_actor2
$ren1 AddActor  probe_actor3
$ren1 AddActor  outline_actor
$ren1 AddActor  plane_outline_actor1
$ren1 AddActor  plane_outline_actor2
$ren1 AddActor  plane_outline_actor3

$ren3 AddVolume comp_volume
$ren3 AddActor  outline_actor



proc change_active_controls { } {
    global control_menu_value

    if [ winfo ismapped .top.f1.f1.f1.comp ] {
	pack forget .top.f1.f1.f1.comp 
    }

    if [ winfo ismapped .top.f1.f1.f1.slice ] {
	pack forget .top.f1.f1.f1.slice 
    }

    if  { $control_menu_value == "Composite Controls" } {
	pack .top.f1.f1.f1.comp -side top -expand 1 -fill both -pady 20
    } elseif  { $control_menu_value == "Slice Viewer Controls" } {
	pack .top.f1.f1.f1.slice -side top -expand 1 -fill both -pady 20
    }
}


proc set_y_slice_value { } {
  global y_slice_value
  global renWin1
  global xdiff ydiff zdiff

  transform2 Identity

  transform2 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  [ expr $zdiff / 2.0 ]

  transform2 RotateX 90

  transform2 Translate \
	  0.0 \
	  0.0 \
	  [expr [expr $ydiff / 2.0 ] - $y_slice_value ] 
 
  transform2 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]
   
  $renWin1 Render
}

proc set_x_slice_value { } {
  global x_slice_value
  global renWin1
  global xdiff ydiff zdiff

  transform3 Identity

  transform3 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  [ expr $zdiff / 2.0 ]

  transform3 RotateY 90

  transform3 Translate \
	  0.0 \
	  0.0 \
	  [expr $x_slice_value - [expr $xdiff / 2.0 ] ] 

  transform3 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]

  $renWin1 Render
}

proc set_z_slice_value { } {
  global z_slice_value
  global renWin1
  global xdiff ydiff zdiff

  transform1 Identity

  transform1 Translate \
	  [ expr $xdiff / 2.0 ] \
	  [ expr $ydiff / 2.0 ] \
	  $z_slice_value

  transform1 Scale     [ expr $xdiff ] [ expr $ydiff ] [ expr $zdiff ]

  $renWin1 Render
}
