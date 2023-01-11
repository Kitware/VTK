#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkCastToConcrete
from vtkmodules.vtkFiltersCore import (
    vtkClipPolyData,
    vtkDecimatePro,
    vtkPolyDataNormals,
    vtkStripper,
    vtkTriangleFilter,
)
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkLinearExtrusionFilter
from vtkmodules.vtkFiltersTexture import vtkTextureMapToPlane
from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkImagingCore import (
    vtkImageConstantPad,
    vtkImageExtractComponents,
    vtkImageShrink3D,
    vtkImageThreshold,
)
from vtkmodules.vtkImagingColor import vtkImageRGBToHSV
from vtkmodules.vtkImagingGeneral import vtkImageGaussianSmooth
from vtkmodules.vtkImagingMorphological import vtkImageSeedConnectivity
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and Interactor
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

imageIn = vtkTIFFReader()
imageIn.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
imageIn.SetOrientationType(4)
imageIn.GetExecutive().SetReleaseDataFlag(0, 0)
imageIn.Update()

def PowerOfTwo (amt):
    ''' Finds the first power of two >= amt. '''
    pow = 0
    amt -= 1
    while True:
        amt = amt >> 1
        pow += 1
        if (amt <= 0):
            return 1 << pow

orgX = imageIn.GetExecutive().GetWholeExtent(
    imageIn.GetOutputInformation(0))[1]\
    - imageIn.GetExecutive().GetWholeExtent(
        imageIn.GetOutputInformation(0))[0] + 1
orgY = imageIn.GetExecutive().GetWholeExtent(
    imageIn.GetOutputInformation(0))[3]\
    - imageIn.GetExecutive().GetWholeExtent(
        imageIn.GetOutputInformation(0))[2] + 1
padX = PowerOfTwo(orgX)
padY = PowerOfTwo(orgY)

imagePowerOf2 = vtkImageConstantPad()
imagePowerOf2.SetInputConnection(imageIn.GetOutputPort())
imagePowerOf2.SetOutputWholeExtent(0, padX - 1, 0, padY - 1, 0, 0)

toHSV = vtkImageRGBToHSV()
toHSV.SetInputConnection(imageIn.GetOutputPort())
toHSV.GetExecutive().SetReleaseDataFlag(0, 0)

extractImage = vtkImageExtractComponents()
extractImage.SetInputConnection(toHSV.GetOutputPort())
extractImage.SetComponents(2)
extractImage.GetExecutive().SetReleaseDataFlag(0, 0)

threshold1 = vtkImageThreshold()
threshold1.SetInputConnection(extractImage.GetOutputPort())
threshold1.ThresholdByUpper(230)
threshold1.SetInValue(255)
threshold1.SetOutValue(0)
threshold1.Update()

extent = threshold1.GetExecutive().GetWholeExtent(
    threshold1.GetOutputInformation(0))

connect = vtkImageSeedConnectivity()
connect.SetInputConnection(threshold1.GetOutputPort())
connect.SetInputConnectValue(255)
connect.SetOutputConnectedValue(255)
connect.SetOutputUnconnectedValue(0)
connect.AddSeed(extent[0], extent[2])
connect.AddSeed(extent[1], extent[2])
connect.AddSeed(extent[1], extent[3])
connect.AddSeed(extent[0], extent[3])

smooth = vtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviation(1, 1)
smooth.SetInputConnection(connect.GetOutputPort())

shrink = vtkImageShrink3D()
shrink.SetInputConnection(smooth.GetOutputPort())
shrink.SetShrinkFactors(2, 2, 1)
shrink.AveragingOn()

geometry = vtkImageDataGeometryFilter()
geometry.SetInputConnection(shrink.GetOutputPort())

geometryTexture = vtkTextureMapToPlane()
geometryTexture.SetInputConnection(geometry.GetOutputPort())
geometryTexture.SetOrigin(0, 0, 0)
geometryTexture.SetPoint1(padX - 1, 0, 0)
geometryTexture.SetPoint2(0, padY - 1, 0)

geometryPD = vtkCastToConcrete()
geometryPD.SetInputConnection(geometryTexture.GetOutputPort())
geometryPD.Update()

clip = vtkClipPolyData()
clip.SetInputData(geometryPD.GetPolyDataOutput())
clip.SetValue(5.5)
clip.GenerateClipScalarsOff()
clip.InsideOutOff()
clip.InsideOutOn()
clip.GetOutput().GetPointData().CopyScalarsOff()
clip.Update()

triangles = vtkTriangleFilter()
triangles.SetInputConnection(clip.GetOutputPort())

decimate = vtkDecimatePro()
decimate.SetInputConnection(triangles.GetOutputPort())
decimate.BoundaryVertexDeletionOn()
decimate.SetDegree(25)
decimate.PreserveTopologyOn()

extrude = vtkLinearExtrusionFilter()
extrude.SetInputConnection(decimate.GetOutputPort())
extrude.SetExtrusionType(2)
extrude.SetScaleFactor(-20)

normals = vtkPolyDataNormals()
normals.SetInputConnection(extrude.GetOutputPort())
normals.SetFeatureAngle(80)

strip = vtkStripper()
strip.SetInputConnection(extrude.GetOutputPort())

map = vtkPolyDataMapper()
map.SetInputConnection(strip.GetOutputPort())
map.SetInputConnection(normals.GetOutputPort())
map.ScalarVisibilityOff()

imageTexture = vtkTexture()
imageTexture.InterpolateOn()
imageTexture.SetInputConnection(imagePowerOf2.GetOutputPort())

clipart = vtkActor()
clipart.SetMapper(map)
clipart.SetTexture(imageTexture)

ren1.AddActor(clipart)

clipart.GetProperty().SetDiffuseColor(1, 1, 1)
clipart.GetProperty().SetSpecular(.5)
clipart.GetProperty().SetSpecularPower(30)
clipart.GetProperty().SetDiffuse(.9)

ren1.ResetCamera()

camera = ren1.GetActiveCamera()
camera.Azimuth(30)
camera.Elevation(-30)
camera.Dolly(1.5)

ren1.ResetCameraClippingRange()
ren1.SetBackground(0.2, 0.3, 0.4)

renWin.SetSize(320, 256)

iren.Initialize()

renWin.Render()

#iren.Start()
