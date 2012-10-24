#!/usr/bin/env python
import vtk
from vtk.test import Testing
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
# Create the reader for the data
# This is the data the will be volume rendered
reader = vtk.vtkStructuredPointsReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/ironProt.vtk")
# create a reader for the other data that will
# be contoured and displayed as a polygonal mesh
reader2 = vtk.vtkSLCReader()
reader2.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/neghip.slc")
# convert from vtkImageData to vtkUnstructuredGrid, remove
# any cells where all values are below 80
thresh = vtk.vtkThreshold()
thresh.ThresholdByUpper(80)
thresh.AllScalarsOff()
thresh.SetInputConnection(reader.GetOutputPort())
# make sure we have only tetrahedra
trifilter = vtk.vtkDataSetTriangleFilter()
trifilter.SetInputConnection(thresh.GetOutputPort())
# Create transfer mapping scalar value to opacity
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(80,0.0)
opacityTransferFunction.AddPoint(120,0.2)
opacityTransferFunction.AddPoint(255,0.2)
# Create transfer mapping scalar value to color
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddRGBPoint(80.0,0.0,0.0,0.0)
colorTransferFunction.AddRGBPoint(120.0,0.0,0.0,1.0)
colorTransferFunction.AddRGBPoint(160.0,1.0,0.0,0.0)
colorTransferFunction.AddRGBPoint(200.0,0.0,1.0,0.0)
colorTransferFunction.AddRGBPoint(255.0,0.0,1.0,1.0)
# The property describes how the data will look
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.ShadeOff()
volumeProperty.SetInterpolationTypeToLinear()
# The mapper / ray cast function know how to render the data
volumeMapper = vtk.vtkUnstructuredGridVolumeRayCastMapper()
volumeMapper.SetInputConnection(trifilter.GetOutputPort())
# The volume holds the mapper and the property and
# can be used to position/orient the volume
volume = vtk.vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)
# contour the second dataset
contour = vtk.vtkContourFilter()
contour.SetValue(0,80)
contour.SetInputConnection(reader2.GetOutputPort())
# create a mapper for the polygonal data
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(contour.GetOutputPort())
mapper.ScalarVisibilityOff()
# create an actor for the polygonal data
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1.AddViewProp(actor)
ren1.AddVolume(volume)
renWin.SetSize(300,300)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(20.0)
ren1.GetActiveCamera().Elevation(10.0)
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
