#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and Interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
imageIn = vtk.vtkTIFFReader()
imageIn.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
imageIn.SetOrientationType(4)
imageIn.GetExecutive().SetReleaseDataFlag(0,0)
imageIn.Update()
def PowerOfTwo (amt,__vtk__temp0=0,__vtk__temp1=0):
    pow = 0
    amt = amt + -1
    while 1:
        amt = expr.expr(globals(), locals(),["amt",">>","1"])
        pow = pow + 1
        if (amt <= 0):
            return expr.expr(globals(), locals(),["1","<<","pow"])
            pass

        pass

orgX = expr.expr(globals(), locals(),["lindex(imageIn.GetExecutive().GetWholeExtent(imageIn.GetOutputInformation(0)),1)","-","lindex(imageIn.GetExecutive().GetWholeExtent(imageIn.GetOutputInformation(0)),0)","+","1"])
orgY = expr.expr(globals(), locals(),["lindex(imageIn.GetExecutive().GetWholeExtent(imageIn.GetOutputInformation(0)),3)","-","lindex(imageIn.GetExecutive().GetWholeExtent(imageIn.GetOutputInformation(0)),2)","+","1"])
padX = PowerOfTwo(orgX)
padY = PowerOfTwo(orgY)
imagePowerOf2 = vtk.vtkImageConstantPad()
imagePowerOf2.SetInputConnection(imageIn.GetOutputPort())
imagePowerOf2.SetOutputWholeExtent(0,expr.expr(globals(), locals(),["padX","-","1"]),0,expr.expr(globals(), locals(),["padY","-","1"]),0,0)
toHSV = vtk.vtkImageRGBToHSV()
toHSV.SetInputConnection(imageIn.GetOutputPort())
toHSV.GetExecutive().SetReleaseDataFlag(0,0)
extractImage = vtk.vtkImageExtractComponents()
extractImage.SetInputConnection(toHSV.GetOutputPort())
extractImage.SetComponents(2)
extractImage.GetExecutive().SetReleaseDataFlag(0,0)
threshold1 = vtk.vtkImageThreshold()
threshold1.SetInputConnection(extractImage.GetOutputPort())
threshold1.ThresholdByUpper(230)
threshold1.SetInValue(255)
threshold1.SetOutValue(0)
threshold1.Update()
extent = threshold1.GetExecutive().GetWholeExtent(threshold1.GetOutputInformation(0))
connect = vtk.vtkImageSeedConnectivity()
connect.SetInputConnection(threshold1.GetOutputPort())
connect.SetInputConnectValue(255)
connect.SetOutputConnectedValue(255)
connect.SetOutputUnconnectedValue(0)
connect.AddSeed(lindex(extent,0),lindex(extent,2))
connect.AddSeed(lindex(extent,1),lindex(extent,2))
connect.AddSeed(lindex(extent,1),lindex(extent,3))
connect.AddSeed(lindex(extent,0),lindex(extent,3))
smooth = vtk.vtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviation(1,1)
smooth.SetInputConnection(connect.GetOutputPort())
shrink = vtk.vtkImageShrink3D()
shrink.SetInputConnection(smooth.GetOutputPort())
shrink.SetShrinkFactors(2,2,1)
shrink.AveragingOn()
geometry = vtk.vtkImageDataGeometryFilter()
geometry.SetInputConnection(shrink.GetOutputPort())
geometryTexture = vtk.vtkTextureMapToPlane()
geometryTexture.SetInputConnection(geometry.GetOutputPort())
geometryTexture.SetOrigin(0,0,0)
geometryTexture.SetPoint1(expr.expr(globals(), locals(),["padX","-","1"]),0,0)
geometryTexture.SetPoint2(0,expr.expr(globals(), locals(),["padY","-","1"]),0)
geometryPD = vtk.vtkCastToConcrete()
geometryPD.SetInputConnection(geometryTexture.GetOutputPort())
geometryPD.Update()
clip = vtk.vtkClipPolyData()
clip.SetInputData(geometryPD.GetPolyDataOutput())
clip.SetValue(5.5)
clip.GenerateClipScalarsOff()
clip.InsideOutOff()
clip.InsideOutOn()
clip.GetOutput().GetPointData().CopyScalarsOff()
clip.Update()
triangles = vtk.vtkTriangleFilter()
triangles.SetInputConnection(clip.GetOutputPort())
decimate = vtk.vtkDecimatePro()
decimate.SetInputConnection(triangles.GetOutputPort())
decimate.BoundaryVertexDeletionOn()
decimate.SetDegree(25)
decimate.PreserveTopologyOn()
extrude = vtk.vtkLinearExtrusionFilter()
extrude.SetInputConnection(decimate.GetOutputPort())
extrude.SetExtrusionType(2)
extrude.SetScaleFactor(-20)
normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(extrude.GetOutputPort())
normals.SetFeatureAngle(80)
strip = vtk.vtkStripper()
strip.SetInputConnection(extrude.GetOutputPort())
map = vtk.vtkPolyDataMapper()
map.SetInputConnection(strip.GetOutputPort())
map.SetInputConnection(normals.GetOutputPort())
map.ScalarVisibilityOff()
imageTexture = vtk.vtkTexture()
imageTexture.InterpolateOn()
imageTexture.SetInputConnection(imagePowerOf2.GetOutputPort())
clipart = vtk.vtkActor()
clipart.SetMapper(map)
clipart.SetTexture(imageTexture)
ren1.AddActor(clipart)
clipart.GetProperty().SetDiffuseColor(1,1,1)
clipart.GetProperty().SetSpecular(.5)
clipart.GetProperty().SetSpecularPower(30)
clipart.GetProperty().SetDiffuse(.9)
ren1.ResetCamera()
camera = ren1.GetActiveCamera()
camera.Azimuth(30)
camera.Elevation(-30)
camera.Dolly(1.5)
ren1.ResetCameraClippingRange()
ren1.SetBackground(0.2,0.3,0.4)
renWin.SetSize(320,256)
iren.Initialize()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
