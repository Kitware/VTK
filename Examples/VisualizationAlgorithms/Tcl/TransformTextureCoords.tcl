#
# This example shows how to generate and manipulate texture coordinates.
# The user can interact with the vtkTransformTextureCoords object to
# modify the texture coordinates interactively. Different objects, textures
# and texture mappers can be selected.
#

#
# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl.
#
package require vtk
package require vtkinteraction

#
# These are the different choices made available to the user.
# They include: models, textures (relative to VTK_DATA_ROOT) 
# and mapper types.
#
set models { \
        "teapot.g" \
        "Viewpoint/cow.g" \
        "motor.g" \
    }

set textures { \
        "vtk.png" \
        "masonry.bmp" \
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
# A 3D model is loaded using an BYU reader. 
# Compute normals, in case they are not provided with the model.
#
vtkBYUReader model_reader
  model_reader SetGeometryFileName "$VTK_DATA_ROOT/Data/[lindex $models 0]"

vtkPolyDataNormals model_normals
  model_normals SetInputConnection [model_reader GetOutputPort]
 
#   
# Create all texture coordinates generators/mappers and use the first one
# for the current pipeline.
#
foreach texture_mapper_type $texture_mapper_types {
    set texture_mapper \
            [$texture_mapper_type [string tolower $texture_mapper_type]]
    $texture_mapper SetInputConnection [model_normals GetOutputPort]
}

#
# Create a texture coordinate transformer, which can be used to 
# translate, scale or flip the texture.
#
set texture_mapper_type [lindex $texture_mapper_types 0]
vtkTransformTextureCoords transform_texture
  transform_texture SetInput [[string tolower $texture_mapper_type] GetOutput]

#
# Create polydata mapper.
#
vtkPolyDataMapper mapper
  mapper SetInputConnection [transform_texture GetOutputPort]

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
  texture SetInputConnection [$texture_reader GetOutputPort]
  texture InterpolateOn

vtkActor actor
  actor SetMapper mapper
  actor SetTexture texture

#
# Create a triangle filter that will feed the model geometry to
# the feature edge extractor. Create the corresponding mapper 
# and actor.
#
vtkTriangleFilter triangle_filter
  triangle_filter SetInputConnection [model_normals GetOutputPort]
 
vtkFeatureEdges edges_extractor
  edges_extractor SetInputConnection [triangle_filter GetOutputPort]
  edges_extractor ColoringOff
  edges_extractor BoundaryEdgesOn
  edges_extractor ManifoldEdgesOn
  edges_extractor NonManifoldEdgesOn

vtkPolyDataMapper edges_mapper
  edges_mapper SetInputConnection [edges_extractor GetOutputPort]
  edges_mapper SetResolveCoincidentTopologyToPolygonOffset

vtkActor edges_actor
  edges_actor SetMapper edges_mapper
  eval [edges_actor GetProperty] SetColor 0 0 0
  eval [edges_actor GetProperty] SetLineStipplePattern 4369
  edges_actor VisibilityOff

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
ren1 AddActor edges_actor
ren1 SetBackground 1 1 1

# 
# Create the Tk widget, associate it with the renderwindow.
#
set vtkw [vtkTkRenderWidget .ren \
        -width 500 \
        -height 400 \
        -rw renWin]

#
# Pack the Tk widget.
#
pack $vtkw -side top -fill both -expand yes

#
# Sets event handlers
#
::vtk::bind_tk_render_widget $vtkw

#
# Create a menubar.
#
set menubar [menu .menubar]
. config -menu $menubar

#
# Create a "File" menu.
#
set file_menu [menu  $menubar.file]

$file_menu add command \
        -label "Quit" \
        -command ::vtk::cb_exit

$menubar add cascade -label "File" -menu $file_menu

#
# Create a "Model" menu.
# Each model is a radio menu entry, associated to 
# the load_model callback.
#
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

#
# Create a "Texture" menu.
# Each texture is a radio menu entry, associated to
# the load_texture callback.
#
set texture_menu [menu $menubar.texture]

set gui_vars(texture_reader,filename) \
        [[[texture GetInputConnection 0 0] GetProducer] GetFileName]

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
    texture SetInputConnection [$texture_reader GetOutputPort]
    renWin Render
}

$menubar add cascade -label "Texture" -menu $texture_menu

#
# Create a "Mapper" menu.
# Each mapper type is a radio menu entry, associated to
# the use_texture_mapper_type callback.
#
set texture_mapper_type_menu [menu $menubar.texture_mapper_type]

set gui_vars(texture_mapper_type) \
        [[[transform_texture GetInputConnection 0 0] GetProducer] GetClassName]

foreach texture_mapper_type $texture_mapper_types {
    $texture_mapper_type_menu add radio \
            -label $texture_mapper_type \
            -command [list use_texture_mapper_type $texture_mapper_type] \
            -value $texture_mapper_type \
            -variable gui_vars(texture_mapper_type)
}

proc use_texture_mapper_type {texture_mapper_type} {
    transform_texture SetInputConnection \
            [[string tolower $texture_mapper_type] GetOutputPort]
    renWin Render
}

$menubar add cascade -label "Mapper" -menu $texture_mapper_type_menu

#
# Create a "View" menu.
# It stores various properties.
#
set view_menu [menu $menubar.view]

set gui_vars(view,edges) [edges_actor GetVisibility]

$view_menu add radio \
        -label "Edges" \
        -command toggle_edges_visibility \
        -value 1 \
        -variable gui_vars(view,edges)

proc toggle_edges_visibility {} {
    if {[edges_actor GetVisibility]} {
        edges_actor VisibilityOff
    } else {
        edges_actor VisibilityOn
    }
    set gui_vars(view,edges) [edges_actor GetVisibility]
    renWin Render
}

$menubar add cascade -label "View" -menu $view_menu

#
# Create the vtkTransformTextureCoords gui.
#
# Each entry in the following array describe a "control" in the GUI:
#       - unique name of the control,
#       - title for the control,
#       - texture coordinates parametrized by that control,
#       - name of the corresponding vtkTransformTextureCoords attribute,
#       - start, end, increment value of each Tk scale widget in the control.
#
set transform_texture_coords_gui_controls \
        { \
            position "Texture position" {r s} Position 0.0 2.0 0.01 \
            scale "Texture scale" {r s} Scale 0.0 5.0 0.05 \
            origin "Texture origin" {r s} Origin 0.0 1.0 0.01 \
        }

proc create_transform_texture_coords_gui {parent obj} {

    global gui_vars transform_texture_coords_gui_controls

    #
    # Create a main frame
    #
    if {$parent == "."} {
        set main_frame [frame .main]
    } else {
        set main_frame [frame $parent.main]
    }

    set scale_width 9
    set command [list update_transform_texture_from_gui_vars $obj]

    #
    # Loop over each "control" description
    #
    foreach {control label coords obj_method scale_from scale_to scale_res} \
            $transform_texture_coords_gui_controls { 

        #
        # Create a frame for the control, a label for its title, and a
        # sub-frame that will hold all Tk scale widgets.
        #
        upvar ${control}_frame control_frame
        set control_frame [frame $main_frame.$control -relief groove -border 2]
        
        upvar ${control}_label control_label
        set control_label [label $control_frame.label \
                -text "$label:" -anchor w]
        
        upvar ${control}_rst control_rst
        set control_rst [frame $control_frame.rst]
        
        #
        # Add (r,s,t) texture coordinate widgets to the control.
        # Each one is made of a label for the coordinate's name, a label
        # for the coordinate's value and a Tk scale widget to control
        # that value. 
        # All scale widgets are associated to the same callback:
        # update_transform_texture_from_gui_vars
        #
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

            #
            # For "origin", add flip checkbuttons.
            # Pack the 3 (or 5) elements into a single row.
            #
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

            # 
            # Allow the scale widgets to grow when the GUI is expanded.
            #
            grid columnconfigure $control_rst 2 -weight 1
        }
        
        # 
        # Pack everything
        #
        pack $control_frame \
                -side top -fill x -expand true -padx 1 -pady 2

        pack $control_label \
                $control_rst \
                -side top -fill x -expand true
    }

    return $main_frame
}

#
# This callback is used whenever the value of a Tk scale is changed.
# It recovers the gui values from the gui_vars global array, and
# change the corresponding vtkTransformTextureCoords attribute.
# The render window is re-rendered.
#
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

#
# Create the gui and pack it.
#
set gui [create_transform_texture_coords_gui . transform_texture]

pack $gui -side top -anchor s -fill x -expand yes

#
# We set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request..
#
wm title . "Texture mapper/transform demo"
wm protocol . WM_DELETE_WINDOW ::vtk::cb_exit

#
# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish) 
#
tkwait window .
