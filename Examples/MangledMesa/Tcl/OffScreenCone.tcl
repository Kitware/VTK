# This example demonstrates how the off-screen capabilities
# of Mesa can be used in VTK.
# Note that to be able to run this example, you need to compile
# VTK with mangled Mesa. See the README.txt file in this directory 
# for instructions on how this can be done.

package require vtktcl

# Create Mesa specific render window and renderer
# When using Mesa, you should always create Mesa specific renderers,
# render windows, mappers, actors, lights, cameras etc...
# See the Mesa classes in the Rendering directory for a list of
# existing classes.
vtkMesaRenderWindow renWin
# Will render in memory.
renWin OffScreenRenderingOn

vtkMesaRenderer ren
renWin AddRenderer ren
# The light and the camera have to be created and set
# because otherwise, during the first render, VTK will 
# use the graphics factory to create them and end up
# with OpenGL objects (instead of Mesa)
vtkMesaLight mlight
ren AddLight mlight
vtkMesaCamera mcamera
ren SetActiveCamera mcamera

vtkConeSource cone

vtkMesaPolyDataMapper mp
mp SetInput [cone GetOutput]

vtkMesaActor actor
actor SetMapper mp
# The property has to be created and set
# because otherwise, during the first render, VTK will 
# use the graphics factory to create it and end up
# with vtkOpenGLProperty object (instead of Mesa)
vtkMesaProperty mprop
actor SetProperty mprop

ren AddActor actor

renWin Render

# Save the window to a png file
vtkWindowToImageFilter w2if
w2if SetInput renWin

vtkPNGWriter wr
wr SetInput [w2if GetOutput]
wr SetFileName "MangledMesaTest.png"
wr Write

# Exit without displaying anything
vtkCommand DeleteAllObjects
exit
