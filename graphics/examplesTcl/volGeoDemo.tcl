catch {load vtktcl}

source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/WidgetObject.tcl
source TkInteractor.tcl

proc CreateDataChoice { ww type } {
    vtkSLCReader reader_$type
    reader_$type SetFileName ../../../vtkdata/$type.slc
    
    vtkVolumeRayCastCompositeFunction composite_$type

    vtkVolumeRayCastMapper mapper_$type
    mapper_$type SetScalarInput [reader_$type GetOutput]
    mapper_$type SetVolumeRayCastFunction composite_$type

    switch $type {
	chair  {set r 0.00; set g 0.83; set b 0.70}
	table  {set r 0.64; set g 0.30; set b 0.00}
	spring {set r 0.35; set g 0.79; set b 0.95}
	nut    {set r 0.65; set g 0.57; set b 0.00}
	bolt   {set r 0.06; set g 0.31; set b 0.91}
	
    }

    vtkColorTransferFunction color_$type
    color_$type AddRGBPoint   0 $r $g $b
    color_$type AddRGBPoint 255 $r $g $b

    vtkPiecewiseFunction opacity_$type
    opacity_$type AddPoint 100 0.0
    opacity_$type AddPoint 150 1.0

    vtkPiecewiseFunction gradopacity_$type
    gradopacity_$type AddPoint   0 0.0
    gradopacity_$type AddPoint   1 0.0
    gradopacity_$type AddPoint   5 1.0

    vtkVolumeProperty property_$type
    property_$type SetColor color_$type
    property_$type SetScalarOpacity opacity_$type
    property_$type SetGradientOpacity gradopacity_$type
    property_$type SetInterpolationTypeToLinear
    property_$type ShadeOn

    vtkVolume volume_$type
    volume_$type SetVolumeMapper mapper_$type
    volume_$type SetVolumeProperty property_$type

    vtkTkRenderWidget $ww.rw_$type -width 100 -height 100

    vtkRenderer ren_choice_$type
    ren_choice_$type SetBackground 0.0 0.0 0.0
    ren_choice_$type AddVolume volume_$type

    [$ww.rw_$type GetRenderWindow] AddRenderer ren_choice_$type
    bind $ww.rw_$type <Expose> {Expose %W}
    bind $ww.rw_$type <Button> \
	    [subst {SetMainWindowOption $type; .geo.right.main Render}]
    pack $ww.rw_$type -side top -expand 1 -fill both
}

proc CreateMainWindowOption { type } {
    vtkRenderer ren_main_$type
    ren_main_$type SetBackground 0.0 0.0 0.0
    ren_main_$type AddVolume volume_$type
}

proc SetMainWindowOption { type } {
    global current_type ambient specular diffuse specularpower

    [[.geo.right.main GetRenderWindow] GetRenderers] RemoveAllItems
    [.geo.right.main GetRenderWindow] AddRenderer ren_main_$type
    .geo.indicator.${current_type} configure -image ""
    .geo.indicator.${current_type} configure -text ""
    .geo.indicator.${type} configure -image IndicatorImage
    set ambient [property_${type} GetAmbient]
    set diffuse [property_${type} GetDiffuse]
    set specular [property_${type} GetSpecular]
    set specularpower [property_${type} GetSpecularPower]
    set current_type $type
    update
}

wm withdraw .

toplevel .geo -visual best -bg #000000
wm title .geo {Volume Rendering Voxelized Geometry}

set dataList [list chair table spring nut bolt]

frame .geo.dataset -bg #000000
pack .geo.dataset -side left -expand 1 -fill both

frame .geo.indicator -bg #000000 -width 50 -height 500
pack .geo.indicator -side left -expand 1 -fill both

frame .geo.right -bg #000000
pack .geo.right -side left -expand 1 -fill both

image create photo IndicatorImage -file "LeftArrow.ppm"
image create photo ColorImage -file "ColorWheel.ppm"

set y 20

foreach i $dataList {
    CreateDataChoice .geo.dataset $i
    CreateMainWindowOption $i

    label .geo.indicator.${i} -text "" -bg #000000  -fg #000000
    place .geo.indicator.${i} -x 2 -y $y -anchor nw

    if { $i == "chair" } { volume_chair RotateY 180 }
    [ren_choice_$i GetActiveCamera] Azimuth -30
    [ren_choice_$i GetActiveCamera] Elevation 20

    incr y 100
}

vtkRenderWindow renWin
vtkTkRenderWidget .geo.right.main -width 300 -height 300 -rw renWin
[.geo.right.main GetRenderWindow] AddRenderer ren_main_chair
.geo.indicator.chair configure -image IndicatorImage
set current_type chair
set ambient [property_chair GetAmbient]
set diffuse [property_chair GetDiffuse]
set specular [property_chair GetSpecular]
set specularpower [property_chair GetSpecularPower]
BindTkRenderWidget .geo.right.main
pack .geo.right.main -side top -expand 1 -fill both

bind .geo.right.main <Any-ButtonRelease> +ReleaseScript 

proc ReleaseScript {} {
    global current_type
    
    set choicelights [ren_choice_${current_type} GetLights]
    $choicelights InitTraversal
    set choicelight [$choicelights GetNextItem]
    
    set mainlights [ren_main_${current_type} GetLights]
    $mainlights InitTraversal
    set mainlight [$mainlights GetNextItem]
    
    if { $mainlight != "" } {
	eval $choicelight SetPosition [$mainlight GetPosition]
    }

    .geo.dataset.rw_${current_type} Render
}

frame .geo.right.controls -bg #000000
pack .geo.right.controls -side top -expand 1 -fill both

label .geo.right.colorwheel -image ColorImage -bg #000000
pack .geo.right.colorwheel -side left -expand 1 -fill both -padx 10

bind .geo.right.colorwheel <Button-1> {
    global current_type

    scan [ColorImage get %x %y] "%%f %%f %%f" r g b
    
    set r [expr $r / 255.0]
    set g [expr $g / 255.0]
    set b [expr $b / 255.0]                 

    color_${current_type} AddRGBPoint 0 $r $g $b
    color_${current_type} AddRGBPoint 255 $r $g $b

    .geo.right.main Render
    .geo.dataset.rw_${current_type} Render
}

proc SetProperties { } {
    global current_type

    property_${current_type} SetAmbient [.geo.right.sliders1.ambient get]
    property_${current_type} SetDiffuse [.geo.right.sliders1.diffuse get]
    property_${current_type} SetSpecular [.geo.right.sliders2.specular get]
    property_${current_type} SetSpecularPower [.geo.right.sliders2.specularpower get]
    .geo.right.main Render
    .geo.dataset.rw_${current_type} Render
}

frame .geo.right.sliders1 -bg #000000
pack .geo.right.sliders1 -side left -expand 1 -fill both

frame .geo.right.sliders2 -bg #000000
pack .geo.right.sliders2 -side left -expand 1 -fill both

scale .geo.right.sliders1.ambient -label Ambient -from 0.0 -to 1.0 \
    -variable ambient -orient horizontal -bg #000000 -fg #aaaaaa \
    -troughcolor #224488 -resolution 0.05 \
    -highlightthickness 0 -activebackground #777777 
pack .geo.right.sliders1.ambient -side top -expand 1 \
    -fill both -padx 5 -pady 2
bind .geo.right.sliders1.ambient <ButtonRelease> { SetProperties }

scale .geo.right.sliders1.diffuse -label Diffuse -from 0.0 -to 1.0 \
    -variable diffuse -orient horizontal -bg #000000 -fg #aaaaaa \
    -troughcolor #224488 -resolution 0.05 \
    -highlightthickness 0 -activebackground #777777 
pack .geo.right.sliders1.diffuse -side top -expand 1 \
    -fill both -padx 5 -pady 2
bind .geo.right.sliders1.diffuse <ButtonRelease> { SetProperties }

scale .geo.right.sliders2.specular -label Specular -from 0.0 -to 1.0 \
    -variable specular -orient horizontal -bg #000000 -fg #aaaaaa \
    -troughcolor #224488 -resolution 0.05 \
    -highlightthickness 0 -activebackground #777777 
pack .geo.right.sliders2.specular -side top -expand 1 \
    -fill both -padx 5 -pady 2
bind .geo.right.sliders2.specular <ButtonRelease> { SetProperties }

scale .geo.right.sliders2.specularpower -label Power -from 1 -to 200 \
    -variable specularpower -orient horizontal -bg #000000 -fg #aaaaaa \
    -troughcolor #224488 \
    -highlightthickness 0 -activebackground #777777 
pack .geo.right.sliders2.specularpower -side top -expand 1 \
    -fill both  -padx 5 -pady 2
bind .geo.right.sliders2.specularpower <ButtonRelease> { SetProperties }

button .geo.right.exit -text Exit -command exit \
    -bg #111111 -fg #aaaaaa -activebackground #444444 \
    -activeforeground #aaaaaa -highlightthickness 0 \
    -bd 3 -highlightbackground #444444
pack .geo.right.exit -side left -expand 1 -fill none -padx 15 -pady 2 -anchor se

 foreach i $dataList {
     ren_main_$i SetActiveCamera [ren_choice_$i GetActiveCamera]
 }

