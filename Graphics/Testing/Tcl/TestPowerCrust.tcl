# This program demonstrates and tests the use of vtkPowerCrustSurfaceReconstruction
# The input is a cloud of points sampled from the surface of an object.
# The output is a complete surface, plus the medial surface of the object.

package require vtk
package require vtkinteraction

# Read some points. Use a programmable filter to read them.
#
vtkProgrammableSource pointSource
    pointSource SetExecuteMethod readPoints
proc readPoints {} {
global VTK_DATA_ROOT
    set output [pointSource GetPolyDataOutput]
    vtkPoints points
    $output SetPoints points
    vtkCellArray polys
    $output SetVerts polys

   set file [open "$VTK_DATA_ROOT/Data/cactus.3337.pts" r]
   while { [gets $file line] != -1 } {
      scan $line "%s" firstToken
      if { $firstToken == "p" } {
         scan $line "%s %f %f %f" firstToken x y z
         polys InsertNextCell 1
         polys InsertCellPoint [points InsertNextPoint $x $y $z]
      }
   }
   points Delete; #okay, reference counting
}

# render the raw input points into the first renderer
vtkPolyDataMapper rawpointsmapper
    rawpointsmapper SetInput [pointSource GetPolyDataOutput]
vtkActor rawpointsactor
    rawpointsactor SetMapper rawpointsmapper
    [rawpointsactor GetProperty] SetColor 0 0 0

# construct the surface and render into the second renderer
vtkPowerCrustSurfaceReconstruction surf
    surf SetInput [pointSource GetPolyDataOutput]
vtkPolyDataMapper map
    map SetInput [surf GetOutput]
vtkActor surfaceActor
    surfaceActor SetMapper map
    [surfaceActor GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784
    [surfaceActor GetProperty] SetSpecularColor 1 1 1
    [surfaceActor GetProperty] SetSpecular .4
    [surfaceActor GetProperty] SetSpecularPower 50

# render the medial surface into a third renderer
    surf Update          
# (because GetMedialSurface is not part of the normal pipeline)
vtkPolyDataMapper medialmapper
    medialmapper SetInput [surf GetMedialSurface]
    medialmapper ScalarVisibilityOff
vtkActor medialactor
    medialactor SetMapper medialmapper
    [medialactor GetProperty] SetDiffuseColor 0.1000 0.8882 0.2784
    [medialactor GetProperty] SetSpecularColor 1 1 1
    [medialactor GetProperty] SetSpecular .4
    [medialactor GetProperty] SetSpecularPower 50
    [medialactor GetProperty] SetRepresentationToWireframe

# Render everything
vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin SetSize 600 200
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer
ren1 AddActor rawpointsactor
ren2 AddActor surfaceActor
ren3 AddActor medialactor

# set the properties of the renderers

ren1 SetBackground 1 1 1
ren1 SetViewport 0.0 0.0 0.33 1.0
[ren1 GetActiveCamera] SetPosition 1 -1 0
ren1 ResetCamera

ren2 SetBackground 1 1 1
ren2 SetViewport 0.33 0.0 0.66 1.0
[ren2 GetActiveCamera] SetPosition 1 -1 0   
ren2 ResetCamera

ren3 SetBackground 1 1 1
ren3 SetViewport 0.66 0.0 1.0 1.0
[ren3 GetActiveCamera] SetPosition 1 -1 0
ren3 ResetCamera

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# output the image to file (used to generate the initial regression image)
#vtkWindowToImageFilter to_image
#to_image SetInput renWin
#vtkPNGWriter to_png
#to_png SetFileName "TestPowerCrust.png"
#to_png SetInput [to_image GetOutput]
#to_png Write

# prevent the tk window from showing up then start the event loop
wm withdraw .


