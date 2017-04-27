#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

lut = vtk.vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Read the data: a height field results
demReader = vtk.vtkDEMReader()
demReader.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

surface = vtk.vtkImageDataGeometryFilter()
surface.SetInputConnection(demReader.GetOutputPort())

warp = vtk.vtkWarpScalar()
warp.SetInputConnection(surface.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)
warp.Update()

normals = vtk.vtkPolyDataNormals()
normals.SetInputData(warp.GetPolyDataOutput())
normals.SetFeatureAngle(60)
normals.SplittingOff()

demMapper = vtk.vtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

demActor = vtk.vtkLODActor()
demActor.SetMapper(demMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(demActor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

ren1.SetBackground(0.1, 0.2, 0.4)
# render the image
#
renWin.Render()

view1 = vtk.vtkCamera()
view1.SetClippingRange(30972.2, 35983.7)
view1.SetFocalPoint(562835, 5.11498e+006, 2294.5)
view1.SetPosition(562835, 5.11498e+006, 35449.9)
view1.SetViewAngle(30)
view1.SetViewUp(0, 1, 0)

view2 = vtk.vtkCamera()
view2.SetClippingRange(9013.43, 13470.4)
view2.SetFocalPoint(562835, 5.11498e+006, 2294.5)
view2.SetPosition(562835, 5.11498e+006, 13269.4)
view2.SetViewAngle(30)
view2.SetViewUp(0, 1, 0)

view3 = vtk.vtkCamera()
view3.SetClippingRange(4081.2, 13866.4)
view3.SetFocalPoint(562853, 5.11586e+006, 2450.05)
view3.SetPosition(562853, 5.1144e+006, 10726.6)
view3.SetViewAngle(30)
view3.SetViewUp(0, 0.984808, 0.173648)

view4 = vtk.vtkCamera()
view4.SetClippingRange(14.0481, 14048.1)
view4.SetFocalPoint(562880, 5.11652e+006, 2733.15)
view4.SetPosition(562974, 5.11462e+006, 6419.98)
view4.SetViewAngle(30)
view4.SetViewUp(0.0047047, 0.888364, 0.459116)

view5 = vtk.vtkCamera()
view5.SetClippingRange(14.411, 14411)
view5.SetFocalPoint(562910, 5.11674e+006, 3027.15)
view5.SetPosition(562414, 5.11568e+006, 3419.87)
view5.SetViewAngle(30)
view5.SetViewUp(-0.0301976, 0.359864, 0.932516)

interpolator = vtk.vtkCameraInterpolator()
interpolator.SetInterpolationTypeToSpline()
interpolator.AddCamera(0, view1)
interpolator.AddCamera(5, view2)
interpolator.AddCamera(7.5, view3)
interpolator.AddCamera(9.0, view4)
interpolator.AddCamera(11.0, view5)

camera = vtk.vtkCamera()
ren1.SetActiveCamera(camera)

def animate():
    numSteps = 500
    min = interpolator.GetMinimumT()
    max = interpolator.GetMaximumT()
    i = 0
    while i <= numSteps:
        t = float(i) * (max - min) / float(numSteps)
        interpolator.InterpolateCamera(t, camera)
        renWin.Render()
        i += 1


interpolator.InterpolateCamera(8.2, camera)

# animate()

#iren.Start()
