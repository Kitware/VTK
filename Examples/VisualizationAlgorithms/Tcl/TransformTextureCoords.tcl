#
# This example shows how to generate and manipulate texture coordinates.
# A random cloud of points is generated and then triangulated with 
# vtkDelaunay3D. Since these points do not have texture coordinates,
# we generate them with vtkTextureMapToCylinder.
#

#
# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl.
#
package require vtk
package require vtkinteraction

#
# Settings. 
# (models, textures, mapper types)
#
set models { \
        "teapot.g" \
        "Viewpoint/cow.g" \
        "motor.g" \
    }

set textures { \
        "masonry.bmp" \
        "vtk.png" \
        "earth.ppm" \
        "B.pgm" \
        "beach.jpg" \
        "fran_cut.png" \
    }

set texture_mapper_types { \
        vtkTextureMapToPlane \
        vtkTextureMapToSphere \
        vtkTextureMapToCylinder \
    }

#
# Read 3D model.
# Compute normals.
#
vtkBYUReader model_reader
  model_reader SetGeometryFileName "$VTK_DATA_ROOT/Data/[lindex $models 0]"

vtkPolyDataNormals model_normals
  model_normals SetInput [model_reader GetOutput]
 
#   
# Create all texture coordinates generators.
#
foreach texture_mapper_type $texture_mapper_types {
    set texture_mapper \
            [$texture_mapper_type [string tolower $texture_mapper_type]]
    $texture_mapper SetInput [model_normals GetOutput]
    # $texture_mapper PreventSeamOn
}

#
# Create texture coordinate transformer.
#
set texture_mapper_type [lindex $texture_mapper_types 0]
vtkTransformTextureCoords transform_texture
  transform_texture SetInput [[string tolower $texture_mapper_type] GetOutput]

#
# Create polydata mapper.
#
vtkPolyDataMapper mapper
  mapper SetInput [transform_texture GetOutput]

#
# A texture is loaded using an image reader. 
# Textures are simply images.
# The texture is eventually associated with an actor.
#
set filename "$VTK_DATA_ROOT/Data/[lindex $textures 0]"
vtkImageReader2Factory create_reader
set texture_reader [create_reader CreateImageReader2 $filename]
$texture_reader SetFileName $filename

vtkTexture texture
  texture SetInput [$texture_reader GetOutput]
  texture InterpolateOn

vtkActor actor
  actor SetMapper mapper
  actor SetTexture texture

#
# Create the standard rendering stuff.
#
vtkRenderer ren1

vtkRenderWindow renWin
  renWin AddRenderer ren1

#
# Add the actors to the renderer, set the background
#
ren1 AddActor actor
ren1 SetBackground 1 1 1

# 
# Create the Tk widget, associate it with the renderwindow.
#
set vtkw [vtkTkRenderWidget .ren \
        -width 500 \
        -height 400 \
        -rw renWin]

pack $vtkw -side top -fill both -expand yes

#
# BindTkRenderWidget sets events handlers that are similar
# to what we would achieve using vtkRenderWindowInteractor.
#
BindTkRenderWidget $vtkw

#
# Create menubar
#
set menubar [menu .menubar]
. config -menu $menubar

set file_menu [menu  $menubar.file]

$file_menu add command \
        -label "Quit" \
        -command bye

proc bye {} {
    vtkCommand DeleteAllObjects
    exit
}

$menubar add cascade -label "File" -menu $file_menu

# Create "Model" menubar

set model_menu [menu $menubar.model]

set gui_vars(model_reader,filename) \
        [model_reader GetGeometryFileName]

foreach model $models {
    set filename "$VTK_DATA_ROOT/Data/$model"
    $model_menu add radio \
            -label $model \
            -command [list load_model $filename] \
            -value $filename \
            -variable gui_vars(model_reader,filename)
}

proc load_model {filename} {
    model_reader SetGeometryFileName $filename
    ren1 ResetCamera
    renWin Render
}

$menubar add cascade -label "Model" -menu $model_menu

# Create "Texture" menubar

set texture_menu [menu $menubar.texture]

set gui_vars(texture_reader,filename) \
        [[[texture GetInput] GetSource] GetFileName]

foreach texture $textures {
    set filename "$VTK_DATA_ROOT/Data/$texture"
    $texture_menu add radio \
            -label $texture \
            -command [list load_texture $filename] \
            -value $filename \
            -variable gui_vars(texture_reader,filename)
}

proc load_texture {filename} {
    set texture_reader [create_reader CreateImageReader2 $filename]
    $texture_reader SetFileName $filename
    texture SetInput [$texture_reader GetOutput]
    renWin Render
}

$menubar add cascade -label "Texture" -menu $texture_menu

# Create "Mapper" menubar

set texture_mapper_type_menu [menu $menubar.texture_mapper_type]

set gui_vars(texture_mapper_type) \
        [[[transform_texture GetInput] GetSource] GetClassName]

foreach texture_mapper_type $texture_mapper_types {
    $texture_mapper_type_menu add radio \
            -label $texture_mapper_type \
            -command [list use_texture_mapper_type $texture_mapper_type] \
            -value $texture_mapper_type \
            -variable gui_vars(texture_mapper_type)
}

proc use_texture_mapper_type {texture_mapper_type} {
    transform_texture SetInput \
            [[string tolower $texture_mapper_type] GetOutput]
    renWin Render
}

$menubar add cascade -label "Mapper" -menu $texture_mapper_type_menu

#
#
# Create the texture coords transformer gui
#
set transform_texture_coords_gui_controls \
        { \
            position "Texture position" {r s} Position 0.0 2.0 0.01 \
            scale "Texture scale" {r s} Scale 0.0 5.0 0.05 \
            origin "Texture origin" {r s} Origin 0.0 1.0 0.01 \
        }

proc create_transform_texture_coords_gui {parent obj} {

    global gui_vars transform_texture_coords_gui_controls

    if {$parent == "."} {
        set main_frame [frame .main]
    } else {
        set main_frame [frame $parent.main]
    }

    set scale_width 9
    set command [list update_transform_texture_from_gui_vars $obj]

    foreach {control label coords obj_method scale_from scale_to scale_res} \
            $transform_texture_coords_gui_controls { 

        # Controls (frame: label + frame/grid)
        
        upvar ${control}_frame control_frame
        set control_frame [frame $main_frame.$control -relief groove -border 2]
        
        upvar ${control}_label control_label
        set control_label [label $control_frame.label \
                -text "$label:" -anchor w]
        
        upvar ${control}_rst control_rst
        set control_rst [frame $control_frame.rst]
        
        # Add (r,s,t) components to each controls

        for {set i 0} {$i < [llength $coords]} {incr i} {

            set coord [lindex $coords $i]

            label $control_rst.${coord}_label \
                    -text "$coord:" -anchor w

            set gui_vars($obj,$control,$coord) \
                    [lindex [$obj Get$obj_method] $i]

            scale $control_rst.${coord}_scale \
                    -from $scale_from -to $scale_to -resolution $scale_res  \
                    -orient horizontal \
                    -width $scale_width \
                    -showvalue false \
                    -var gui_vars($obj,$control,$coord) \
                    -command $command

            label $control_rst.${coord}_value \
                    -textvariable gui_vars($obj,$control,$coord)

            # For "origin", add flip controls

            if {$control == "origin"} {

                label $control_rst.${coord}_flip_label \
                        -text "Flip:" -anchor w

                set get_flip "GetFlip[string toupper $coord]"
                set gui_vars($obj,$control,${coord}_flip) [$obj $get_flip]

                checkbutton $control_rst.${coord}_flip \
                        -variable gui_vars($obj,$control,${coord}_flip) \
                        -borderwidth 0 -padx 0 -pady 0 \
                        -command $command

                grid $control_rst.${coord}_label \
                        $control_rst.${coord}_value \
                        $control_rst.${coord}_scale \
                        $control_rst.${coord}_flip_label \
                        $control_rst.${coord}_flip \
                        -sticky news 
            } else {
                grid $control_rst.${coord}_label \
                        $control_rst.${coord}_value \
                        $control_rst.${coord}_scale \
                        -sticky news 
            }

            grid columnconfigure $control_rst 2 -weight 1
        }
        
        pack $control_frame \
                -side top -fill x -expand true -padx 1 -pady 2

        pack $control_label \
                $control_rst \
                -side top -fill x -expand true
    }

    return $main_frame
}

proc update_transform_texture_from_gui_vars {obj args} {

    global gui_vars transform_texture_coords_gui_controls

    foreach {control label coords obj_method scale_from scale_to scale_res} \
            $transform_texture_coords_gui_controls { 
        set values [$obj Get$obj_method]
        for {set i 0} {$i < [llength $coords]} {incr i} {
            set coord [lindex $coords $i]
            set values [lreplace $values $i $i $gui_vars($obj,$control,$coord)]

            if {$control == "origin"} {
                set flip_method "Flip[string toupper $coord]"
                $obj Set$flip_method $gui_vars($obj,$control,${coord}_flip)
            }
        }
        eval $obj Set$obj_method $values
    }
    renWin Render
}

set gui [create_transform_texture_coords_gui . transform_texture]

pack $gui -side top -anchor s -fill x -expand yes

#
# We set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request..
#
wm protocol . WM_DELETE_WINDOW bye 

#
# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish) 
#
tkwait window .

