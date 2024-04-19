#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
)
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPiecewiseFunction,
)
from vtkmodules.vtkFiltersCore import (
    vtkDelaunay3D,
    vtkProbeFilter,
)
from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkRenderingCore import (
    vtkColorTransferFunction,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkVolume,
    vtkVolumeProperty,
)
from vtkmodules.vtkRenderingVolumeOpenGL2 import vtkSmartVolumeMapper
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

dim = 48
center = [float(dim)/2.0]*3
extent = [ 0, dim - 1, 0, dim - 1, 0, dim - 1]

imageSource = vtkRTAnalyticSource()
imageSource.SetWholeExtent(extent[0], extent[1], extent[2], extent[3],
                           extent[4], extent[5])
imageSource.SetCenter(center)
imageSource.Update()

img = imageSource.GetOutput()
scalarRange = img.GetScalarRange()
origin = img.GetOrigin()
spacing = img.GetSpacing()


# create an unstructured grid by generating a point cloud and
# applying Delaunay triangulation on it.
vtkMath().RandomSeed(0) # vtkPointSource internally uses vtkMath::Random()
pointSource = vtkPointSource()
pointSource.SetCenter(center)
pointSource.SetRadius(center[0])
pointSource.SetNumberOfPoints(24 * 24 * 24)

delaunay3D = vtkDelaunay3D()
delaunay3D.SetInputConnection(pointSource.GetOutputPort())

# probe into img using unstructured grif geometry
probe1 = vtkProbeFilter()
probe1.SetSourceData(img)
probe1.SetInputConnection(delaunay3D.GetOutputPort())

# probe into the unstructured grid using ImageData geometry
outputData = vtkImageData()
outputData.SetExtent(extent)
outputData.SetOrigin(origin)
outputData.SetSpacing(spacing)
fa = vtkFloatArray()
fa.SetName("scalars")
fa.Allocate(dim ** 3)
outputData.GetPointData().SetScalars(fa)

probe2 = vtkProbeFilter()
probe2.SetSourceConnection(probe1.GetOutputPort())
probe2.SetInputData(outputData)


# render using ray-cast volume rendering
ren = vtkRenderer()
renWin = vtkRenderWindow()
iren = vtkRenderWindowInteractor()
renWin.AddRenderer(ren)
iren.SetRenderWindow(renWin)

volumeMapper = vtkSmartVolumeMapper()
volumeMapper.SetInputConnection(probe2.GetOutputPort())
volumeMapper.SetRequestedRenderModeToRayCast()

volumeColor = vtkColorTransferFunction()
volumeColor.AddRGBPoint(scalarRange[0], 0.0, 0.0, 1.0)
volumeColor.AddRGBPoint((scalarRange[0] + scalarRange[1]) * 0.5, 0.0, 1.0, 0.0)
volumeColor.AddRGBPoint(scalarRange[1], 1.0, 0.0, 0.0)

volumeScalarOpacity = vtkPiecewiseFunction()
volumeScalarOpacity.AddPoint(scalarRange[0], 0.0)
volumeScalarOpacity.AddPoint((scalarRange[0] + scalarRange[1]) * 0.5, 0.0)
volumeScalarOpacity.AddPoint(scalarRange[1], 1.0)

volumeProperty = vtkVolumeProperty()
volumeProperty.SetColor(volumeColor)
volumeProperty.SetScalarOpacity(volumeScalarOpacity)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOn()
volumeProperty.SetAmbient(0.5)
volumeProperty.SetDiffuse(0.8)
volumeProperty.SetSpecular(0.2)

volume = vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

ren.AddViewProp(volume)
ren.ResetCamera()
renWin.SetSize(300, 300)
renWin.Render()
