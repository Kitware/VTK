#
# This example creates a polygonal model of a mace made of a sphere
# and a set of cones adjusted on its surface using glyphing. 
#
#
# The sphere is rendered to the screen through the usual VTK render
# window but is included inside a standard Tk GUI comprising several
# other Tk widgets allowing the user to modify the VTK objects
# properties interactively. Interactions are performed through Tk
# events bindings instead of vtkRenderWindowInteractor.
#

#
# First we include the vtktcl package which will make available 
# all of the vtk commands to Tcl
#
package require vtktcl_widgets

#
# Next we create an instance of vtkSphereSource and set some of its 
# properties
#
vtkSphereSource sphere
    sphere SetThetaResolution 8 
    sphere SetPhiResolution 8

#
# We create an instance of vtkPolyDataMapper to map the polygonal data 
# into graphics primitives. We connect the output of the sphere source
# to the input of this mapper 
#
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]

#
# Create an actor to represent the sphere. The actor coordinates rendering of
# the graphics primitives for a mapper. We set this actor's mapper to be
# the mapper which we created above.
#
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

#
# Next we create an instance of vtkConeSource that will be used to
# set the glyphs on the sphere's surface
#
vtkConeSource cone
    cone SetResolution 6

#
# Glyphing is a visualization technique that represents data by using
# symbol or glyphs. In VTK, the vtkGlyph3D class allows you to create
# glyphs that can be scaled, colored and oriented along a
# direction. The glyphs (here, cones) are copied at each point of the
# input dataset (the sphere's vertices).
#
# Create a vtkGlyph3D to dispatch the glyph/cone geometry (SetSource) on the
# sphere dataset (SetInput). Each glyph is oriented through the dataset 
# normals (SetVectorModeToUseNormal). The resulting dataset is a set
# of cones lying on a sphere surface.
#
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal 
    glyph SetScaleModeToScaleByVector 
    glyph SetScaleFactor 0.25

#
# We create an instance of vtkPolyDataMapper to map the polygonal data 
# into graphics primitives. We connect the output of the glyph3d
# to the input of this mapper 
#
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]

#
# Create an actor to represent the glyphs. The actor coordinates rendering of
# the graphics primitives for a mapper. We set this actor's mapper to be
# the mapper which we created above.
#
vtkActor spikeActor
    spikeActor SetMapper spikeMapper

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has. We also set the background color here.
#
vtkRenderer renderer
    renderer AddActor sphereActor
    renderer AddActor spikeActor
    renderer SetBackground 1 1 1

#
# We create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. 
# Do not set the size of the window here.
#
vtkRenderWindow renWin
    renWin AddRenderer renderer

#
# vtkTkRenderWidget is a Tk widget that we can render into. It has a
# GetRenderWindow method that returns a vtkRenderWindow. This can then
# be used to create a vtkRenderer and etc. We can also specify a
# vtkRenderWindow to be used when creating the widget by using the -rw
# option, which is what we do here by using renWin. It also takes
# -width and -height options that can be used to specify the widget
# size, hence the render window size: we set the size to be 300
# pixels by 300. 
#
set vtkw [vtkTkRenderWidget .ren \
        -width 300 \
        -height 300 \
        -rw renWin]

#
# When using the vtkTkWidget classes you should not use the interactor
# classes such as vtkRenderWindowInteractor. While it may work, the
# behavior may be unstable. Normally you should use either an
# interactor (see Mace.Tcl) or a vtkTkWidget but never both for a
# given window. Hopefully, events can be bound on this widget just like any
# other Tk widget. BindTkRenderWidget sets events handlers that are similar
# to what we would achieve using vtkRenderWindowInteractor.
#
BindTkRenderWidget $vtkw

#
# Once the VTK widget has been created it can be inserted into a whole Tk GUI
# as well as any other standard Tk widgets. The code below will create several
# "scale" (sliders) widgets enabling us to control the mace parameters 
# interactively.
#

#
# We first create a .params Tk frame into which we will pack all sliders.
#

frame .params

#
# Next we create a scale slider controlling the sphere Theta
# resolution. The value of this slider will range from 3 to 20 by
# increment 1 (-from, -to and -res options respectively). The
# orientation of this widget is horizontal (-orient option). We label
# it using the -label option. Finally, we bind the scale to Tcl code
# by assigning the -command option to the name of a Tcl
# procedure. Whenever the slider value changes this procedure will be
# called, enabling us to propagate this GUI setting to the
# corresponding VTK object.
#
set sth [scale .params.sth \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Sphere Theta Resolution:" \
        -command setSphereThetaResolution]

#
# The slider widget is initialized using the value obtained from the 
# corresponding VTK object (i.e. the sphere theta resolution).
#
$sth set [sphere GetThetaResolution]

#
# The procedure is called by the slider-widget handler whenever the
# slider value changes (either through user interaction or
# programmatically). It receives the slider value as parameter. We
# update the corresponding VTK object by calling the
# SetThetaResolution using this parameter and we render the scene to
# update the pipeline.
#
proc setSphereThetaResolution {res} {
    sphere SetThetaResolution $res
    renWin Render
}

#
# In the exact same way we create a scale slider controlling the sphere Phi
# resolution. 
#
set sph [scale .params.sph \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Sphere Phi Resolution:" \
        -command setSpherePhiResolution]

$sph set [sphere GetPhiResolution]

proc setSpherePhiResolution {res} {
    sphere SetPhiResolution $res
    renWin Render
}

#
# In the exact same way we create a scale slider controlling the cone
# resolution. 
#
set cre [scale .params.cre \
        -from 3 -to 20 -res 1 \
        -orient horizontal \
        -label "Cone Source Resolution:" \
        -command setConeSourceResolution]

$cre set [cone GetResolution]

proc setConeSourceResolution {res} {
    cone SetResolution $res
    renWin Render
}

#
# In the exact same way we create a scale slider controlling the glyph
# scale factor. 
#
set gsc [scale .params.gsc \
        -from 0.1 -to 1.5 -res 0.05 \
        -orient horizontal\
        -label "Glyph Scale Factor:" \
        -command setGlyphScaleFactor]
$gsc set [glyph GetScaleFactor]

proc setGlyphScaleFactor {factor} {
    glyph SetScaleFactor $factor
    renWin Render
}


#
# Finally we pack all sliders on top of each other (-side top) inside
# the frame and we pack the VTK widget $vtkw and the frame .params
# inside the main root widget.
#
pack $sth $sph $cre $gsc -side top -anchor nw -fill both
pack $vtkw .params -side top -fill both -expand yes

#
# We set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request. This
# request is triggered when the widget is closed using the standard
# window manager icons or buttons. In this case the 'bye' procedure
# will be called and it will free up any objects we created then exit
# the application.
#
wm protocol . WM_DELETE_WINDOW bye 
proc bye {} {
    vtkCommand DeleteAllObjects
    exit
}

