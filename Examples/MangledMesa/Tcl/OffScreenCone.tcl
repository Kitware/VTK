# This example demonstrates how the off-screen capabilities
# of Mesa can be used in VTK.
# Note that to be able to run this example, you need to compile
# VTK with mangled Mesa. See the README.txt file in this directory 
# for instructions on how this can be done.

package require vtk
package require vtkinteraction

# Create Mesa specific render window and renderer
# When using Mesa, you should always create Mesa specific renderers,
# render windows, mappers, actors, lights, cameras etc...
# See the Mesa classes in the Rendering directory for a list of
# existing classes.
vtkXMesaRenderWindow renWin
# Will render in memory.
renWin OffScreenRenderingOn

vtkMesaRenderer ren
renWin AddRenderer ren

vtkConeSource cone

vtkMesaPolyDataMapper mp
mp SetInputConnection [cone GetOutputPort]

vtkMesaActor actor
actor SetMapper mp

ren AddActor actor

renWin Render

# Save the window to a png file
vtkWindowToImageFilter w2if
w2if SetInput renWin

vtkPNGWriter wr
wr SetInputConnection [w2if GetOutputPort]
wr SetFileName "MangledMesaTest.png"
wr Write

# Exit without displaying anything
vtkCommand DeleteAllObjects
exit
