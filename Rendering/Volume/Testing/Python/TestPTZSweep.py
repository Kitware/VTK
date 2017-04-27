#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the standard renderer, render window
# and interactor
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.SetDesiredUpdateRate(3)
# Create a small mesh.  The coarser and more opaque the mesh, the easier it
# is to see rendering errors.
input = vtk.vtkImageMandelbrotSource()
input.SetWholeExtent(0,2,0,2,0,2)
input.SetSizeCX(2,2,2,2)
input.SetMaximumNumberOfIterations(10)
# make sure we have only tetrahedra
trifilter = vtk.vtkDataSetTriangleFilter()
trifilter.SetInputConnection(input.GetOutputPort())
# Create transfer mapping scalar value to opacity
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(0,0.0)
opacityTransferFunction.AddPoint(10,1.0)
# Create transfer mapping scalar value to color
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddRGBPoint(0,1.0,0.0,1.0)
colorTransferFunction.AddRGBPoint(2,0.0,0.0,1.0)
colorTransferFunction.AddRGBPoint(4,0.0,1.0,1.0)
colorTransferFunction.AddRGBPoint(6,0.0,1.0,0.0)
colorTransferFunction.AddRGBPoint(8,1.0,1.0,0.0)
colorTransferFunction.AddRGBPoint(10,1.0,0.0,0.0)
# The property describes how the data will look
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.ShadeOff()
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.SetScalarOpacityUnitDistance(0.75)
# The mapper / ray cast function / ray integrator know how to render the data
volumeMapper = vtk.vtkUnstructuredGridVolumeZSweepMapper()
volumeMapper.SetInputConnection(trifilter.GetOutputPort())
#vtkUnstructuredGridLinearRayIntegrator rayIntegrator
#    volumeMapper SetRayIntegrator rayIntegrator
rayIntegrator = vtk.vtkUnstructuredGridPreIntegration()
volumeMapper.SetRayIntegrator(rayIntegrator)
# The volume holds the mapper and the property and
# can be used to position/orient the volume
volume = vtk.vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)
ren1.AddVolume(volume)
renWin.SetSize(300,300)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(20.0)
ren1.GetActiveCamera().Elevation(15.0)
ren1.GetActiveCamera().Zoom(1.5)
renWin.Render()
def TkCheckAbort (__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent",TkCheckAbort)
iren.Initialize()
# --- end of script --
