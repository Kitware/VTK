catch {load vtktcl}

if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl

# do alpha-blending of two images

vtkPNMReader reader1
reader1 SetFileName "$VTK_DATA/masonry.ppm"

vtkPNMReader reader2
reader2 SetFileName "$VTK_DATA/B.pgm"

vtkTIFFReader reader3
reader3 SetFileName "$VTK_DATA/colorsalpha.tif"

vtkLookupTable table
table SetTableRange 0 127 
table SetValueRange 0.0 1.0 
table SetSaturationRange 0.0 0.0 
table SetHueRange 0.0 0.0 
table SetAlphaRange 0.9 0.0 
table Build

vtkImageMapToColors rgba
rgba SetInput [reader2 GetOutput]
rgba SetLookupTable table 

vtkImageTranslateExtent translate
translate SetInput [rgba GetOutput]
translate SetTranslation 60 60 0

vtkImageBlend blend
blend SetBlendModeToCompound
blend AddInput [reader1 GetOutput]
blend AddInput [reader3 GetOutput]
blend AddInput [translate GetOutput]

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [blend GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0

viewer Render

proc SetOpacity {input opacity} {
    blend SetOpacity $input $opacity
    viewer Render
}

proc SetCompoundThreshold {threshold} {
    blend SetCompoundThreshold $threshold
    viewer Render
}

#make interface
vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]

vtkTIFFWriter tiffWriter
  tiffWriter SetInput [windowToimage GetOutput]
  tiffWriter SetFileName "TestBlendCompound.tcl.tif"
#  tiffWriter Write

source $VTK_TCL/../imaging/examplesTcl/WindowLevelInterface.tcl

# only show ui if not testing
if {[info commands rtExMath] != "rtExMath"} {
    # opacity 
    for {set input 0} {$input < [blend GetNumberOfInputs]} {incr input} {
        set opacity_$input [blend GetOpacity $input]
        scale .wl.opacity_$input \
                -from 0.0 \
                -to 1.0 \
                -orient horizontal \
                -command "SetOpacity $input" \
                -variable opacity_$input \
                -label "Opacity ($input):" \
                -resolution .01 
        pack .wl.opacity_$input -side top  -fill both -expand yes
    }
    set compound_threshold [blend GetCompoundThreshold]
    scale .wl.compound_threshold \
            -from 0.0 \
            -to 1.0 \
            -orient horizontal \
            -command "SetCompoundThreshold" \
            -variable compound_threshold \
            -label "Compound threshold:" \
            -resolution .01 
    pack .wl.compound_threshold -side top  -fill both -expand yes
    # blend mode 
    frame .wl.bmode
    label .wl.bmode.label -text "Blend Mode:"
    pack .wl.bmode.label -side top -anchor sw
    set blend_mode [blend GetBlendModeAsString]
    foreach mode {Normal Compound} {
        set lmode [string tolower $mode]
        radiobutton .wl.bmode.$lmode \
                -text $mode \
                -value $mode \
                -variable blend_mode \
                -command "blend SetBlendModeTo$mode ; viewer Render"
        pack .wl.bmode.$lmode -side top -anchor sw
    }
    pack .wl.bmode -side top -fill both -expand yes
}



