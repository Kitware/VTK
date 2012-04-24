#
# This example creates a polygonal model of a mace made of a sphere
# and a set of cones adjusted on its surface using glyphing.
#
# The sphere is rendered to the screen through the usual VTK render window
# and interactions is performed using vtkRenderWindowInteractor.
# The basic setup of source -> mapper -> actor -> renderer ->
# renderwindow is typical of most VTK programs.
#

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

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
    sphereMapper SetInputConnection [sphere GetOutputPort]

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
# of cones laying on a sphere surface.
#
vtkGlyph3D glyph
    glyph SetInputConnection [sphere GetOutputPort]
    glyph SetSourceConnection [cone GetOutputPort]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25

#
# We create an instance of vtkPolyDataMapper to map the polygonal data
# into graphics primitives. We connect the output of the glyph3d
# to the input of this mapper
#
vtkPolyDataMapper spikeMapper
    spikeMapper SetInputConnection [glyph GetOutputPort]

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
# We put our renderer into the render window using AddRenderer. We also
# set the size to be 300 pixels by 300
#
vtkRenderWindow renWin
    renWin AddRenderer renderer
    renWin SetSize 300 300

#
# Finally we create the render window interactor handling user
# interactions. vtkRenderWindowInteractor provides a
# platform-independent interaction mechanism for mouse/key/time
# events. vtkRenderWindowInteractor also provides controls for
# picking, rendering frame rate, and headlights. It is associated
# to a render window.
#
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#
# vtkRenderWindowInteractor provides default key bindings.  The 'u'
# key will trigger its "user method", provided that it has been
# defined. Similarly the 'e' or 'q' key will trigger its "exit
# method". The lines below set these methods through the AddObserver
# method with the events "UserEvent" and "ExitEvent". The corresponding
# "user-method" Tcl code will bring up the .vtkInteract widget and
# allow the user to evaluate any Tcl code and get access to all
# previously-created VTK objects. The
# "exit-method" Tcl code will exit (do not try to free up any objects
# we created using 'vtkCommand DeleteAllObjects' because you are right
# inside a VTK object.
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren AddObserver ExitEvent {exit}

#
# Render the image
#
renWin Render

#
# Hide the default . widget
#
wm withdraw .

#
# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish)
#
tkwait window .
