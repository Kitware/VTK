# This example shows how to color an isosurface with other data.

package require vtk
package require vtkinteraction

# Read some data. The important thing here is to read a function
# as a data array as well as the scalar and vector. Later this
# data array will be used to color the isosurface.
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d AddFunction 153
    pl3d Update
    pl3d DebugOn
    
vtkContourFilter iso
    iso SetInput [pl3d GetOutput]
    iso SetValue 0 .24

vtkPolyDataNormals normals
    normals SetInput [iso GetOutput]
    normals SetFeatureAngle 45

vtkPolyDataMapper isoMapper
    isoMapper SetInput [normals GetOutput]
    isoMapper ScalarVisibilityOn
    isoMapper SetScalarRange 0 1500
    isoMapper SetScalarModeToUsePointFieldData
    isoMapper ColorByArrayComponent "Velocity Magnitude" 0

vtkLODActor isoActor
    isoActor SetMapper isoMapper
    isoActor SetNumberOfCloudPoints 1000

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName "combColorIso.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


