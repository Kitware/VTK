#!/usr/bin/env python

# This example illustrates how one may explicitly specify the range of each
# axes that's used to define the prop, while displaying data with a different
# set of bounds (unlike cubeAxes2.tcl). This example allows you to separate
# the notion of extent of the axes in physical space (bounds) and the extent
# of the values it represents. In other words, you can have the ticks and
# labels show a different range.
#
# read in an interesting object and outline it
#
fohe = vtk.vtkBYUReader()
fohe.SetGeometryFileName("" + str(VTK_DATA_ROOT) + "/Data/teapot.g")
normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(fohe.GetOutputPort())
foheMapper = vtk.vtkPolyDataMapper()
foheMapper.SetInputConnection(normals.GetOutputPort())
foheActor = vtk.vtkLODActor()
foheActor.SetMapper(foheMapper)
foheActor.GetProperty().SetDiffuseColor(0.7,0.3,0.0)
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())
mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
# Create the RenderWindow, Renderer, and setup viewports
camera = vtk.vtkCamera()
camera.SetClippingRange(1.0,100.0)
camera.SetFocalPoint(0.9,1.0,0.0)
camera.SetPosition(11.63,6.0,10.77)
light = vtk.vtkLight()
light.SetFocalPoint(0.21406,1.5,0)
light.SetPosition(8.3761,4.94858,4.12505)
ren2 = vtk.vtkRenderer()
ren2.SetActiveCamera(camera)
ren2.AddLight(light)
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren2)
renWin.SetWindowName("VTK - Cube Axes custom range")
renWin.SetSize(600,600)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)
ren2.SetBackground(0.1,0.2,0.4)
normals.Update()
bounds = normals.GetOutput().GetBounds()
axes2 = vtk.vtkCubeAxesActor()
axes2.SetBounds(lindex(bounds,0),lindex(bounds,1),lindex(bounds,2),lindex(bounds,3),lindex(bounds,4),lindex(bounds,5))
axes2.SetXAxisRange(20,300)
axes2.SetYAxisRange(-0.01,0.01)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetXLabelFormat("%6.1f")
axes2.SetYLabelFormat("%6.1f")
axes2.SetZLabelFormat("%6.1f")
axes2.SetFlyModeToClosestTriad()
axes2.SetScreenSize(20.0)
ren2.AddViewProp(axes2)
renWin.Render()
ren2.ResetCamera()
renWin.Render()
# render the image
#
iren.Initialize()
def TkCheckAbort (__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent",TkCheckAbort)
# prevent the tk window from showing up then start the event loop
# --- end of script --
