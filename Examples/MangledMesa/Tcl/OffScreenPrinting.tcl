package require vtk
package require vtkinteraction

# This script demonstrates the use of mangled Mesa to generate an 
# off-screen copy of the OpenGL render window. A cone is created
# and added to the OpenGL renderer. When the user press the Print
# button, a copy of the scene is rendered on the Mesa window
# (which is set to render in memory) and then the scene is save to
# a png file using the png writer.

# Create the pipeline
# Note that an equivalent Mesa object has to be created for each
# OpenGL object.

# Turn on the use of the Mesa classes in the graphics factory.
vtkGraphicsFactory gf
gf SetUseMesaClasses 1

# The OpenGL render window
vtkRenderWindow rw
# The Mesa equivalent
vtkXMesaRenderWindow mrw
mrw OffScreenRenderingOn

# OpenGL
vtkRenderer ren
rw AddRenderer ren
# Mesa
vtkMesaRenderer mren
mrw AddRenderer mren

vtkConeSource cone

# OpenGL
vtkPolyDataMapper map
map SetInputConnection [cone GetOutputPort]
# Mesa
vtkMesaPolyDataMapper mmap
mmap SetInputConnection [cone GetOutputPort]

# OpenGL
vtkActor actor
actor SetMapper map
# Mesa
vtkMesaActor mactor
mactor SetMapper mmap

# Add the actor to the renderer
ren AddActor actor
mren AddActor mactor

# These are for creating an image from the Mesa render window
vtkWindowToImageFilter w2if
w2if SetInput mrw

vtkPNGWriter writer
writer SetInputConnection [w2if GetOutputPort]
writer SetFileName "MesaPrintout.png"

set mesaCamera [mren GetActiveCamera]
set openGLCamera [ren GetActiveCamera]

proc PrintWithMesa {} {
    global mesaCamera openGLCamera

    # This is needed to create a default mesa light
    mrw Render

    set mesaLights [mren GetLights]
    eval $mesaLights InitTraversal
    set mesaLight [$mesaLights GetNextItem]
    set openGLLights [ren GetLights]
    eval $openGLLights InitTraversal
    set openGLLight [$openGLLights GetNextItem]

    eval $mesaCamera SetPosition [$openGLCamera GetPosition]
    eval $mesaCamera SetFocalPoint [$openGLCamera GetFocalPoint]
    eval $mesaCamera SetViewUp [$openGLCamera GetViewUp]
    eval $mesaCamera SetClippingRange [$openGLCamera GetClippingRange]

    eval $mesaLight DeepCopy $openGLLight

    mrw Render

    # This is here to force the window to image filter to
    # update it's image. Required.
    w2if Modified

    writer Write
}

# ------------------- Create the UI ---------------------
wm withdraw .

# Create the toplevel window
toplevel .top
wm title .top {Printing with Mesa Offscreen Demo}

# Create two frames
frame .top.f1 
frame .top.f2
pack .top.f1 .top.f2 -side top -expand 1 -fill both

vtkTkRenderWidget .top.f1.rw -width 400 -height 400 -rw rw
::vtk::bind_tk_render_widget .top.f1.rw
pack .top.f1.rw -expand 1 -fill both


button .top.f2.b0 -text "Print" -command {PrintWithMesa}
button .top.f2.b1 -text "Quit" -command ::vtk::cb_exit
pack .top.f2.b0  -expand 1 -fill x
pack .top.f2.b1  -expand 1 -fill x




