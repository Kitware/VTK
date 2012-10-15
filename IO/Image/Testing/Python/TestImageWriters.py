#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
image1 = vtk.vtkTIFFReader()
image1.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
image1.SetOrientationType(4)
image1.Update()
sp = vtk.vtkStructuredPoints()
sp.SetDimensions(image1.GetOutput().GetDimensions())
sp.SetExtent(image1.GetOutput().GetExtent())
sp.SetScalarType(image1.GetOutput().GetScalarType(),image1.GetOutputInformation(0))
sp.SetNumberOfScalarComponents(image1.GetOutput().GetNumberOfScalarComponents(),image1.GetOutputInformation(0))
sp.GetPointData().SetScalars(image1.GetOutput().GetPointData().GetScalars())
luminance = vtk.vtkImageLuminance()
luminance.SetInputData(sp)
#
# write to the temp directory if possible, otherwise use .
#
dir = "."
if (info.commands(globals(), locals(),  "rtTester") == "rtTester"):
    dir = rtTester.GetTempDirectory()
    pass
# make sure it is writeable first
if (catch.catch(globals(),"""channel = open("" + str(dir) + "/test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "" + str(dir) + "/test.tmp")
    tiff1 = vtk.vtkTIFFWriter()
    tiff1.SetInputConnection(image1.GetOutputPort())
    tiff1.SetFileName("" + str(dir) + "/tiff1.tif")
    tiff2 = vtk.vtkTIFFWriter()
    tiff2.SetInputConnection(luminance.GetOutputPort())
    tiff2.SetFileName("" + str(dir) + "/tiff2.tif")
    bmp1 = vtk.vtkBMPWriter()
    bmp1.SetInputConnection(image1.GetOutputPort())
    bmp1.SetFileName("" + str(dir) + "/bmp1.bmp")
    bmp2 = vtk.vtkBMPWriter()
    bmp2.SetInputConnection(luminance.GetOutputPort())
    bmp2.SetFileName("" + str(dir) + "/bmp2.bmp")
    pnm1 = vtk.vtkPNMWriter()
    pnm1.SetInputConnection(image1.GetOutputPort())
    pnm1.SetFileName("" + str(dir) + "/pnm1.pnm")
    pnm2 = vtk.vtkPNMWriter()
    pnm2.SetInputConnection(luminance.GetOutputPort())
    pnm2.SetFileName("" + str(dir) + "/pnm2.pnm")
    psw1 = vtk.vtkPostScriptWriter()
    psw1.SetInputConnection(image1.GetOutputPort())
    psw1.SetFileName("" + str(dir) + "/psw1.ps")
    psw2 = vtk.vtkPostScriptWriter()
    psw2.SetInputConnection(luminance.GetOutputPort())
    psw2.SetFileName("" + str(dir) + "/psw2.ps")
    pngw1 = vtk.vtkPNGWriter()
    pngw1.SetInputConnection(image1.GetOutputPort())
    pngw1.SetFileName("" + str(dir) + "/pngw1.png")
    pngw2 = vtk.vtkPNGWriter()
    pngw2.SetInputConnection(luminance.GetOutputPort())
    pngw2.SetFileName("" + str(dir) + "/pngw2.png")
    jpgw1 = vtk.vtkJPEGWriter()
    jpgw1.SetInputConnection(image1.GetOutputPort())
    jpgw1.SetFileName("" + str(dir) + "/jpgw1.jpg")
    jpgw2 = vtk.vtkJPEGWriter()
    jpgw2.SetInputConnection(luminance.GetOutputPort())
    jpgw2.SetFileName("" + str(dir) + "/jpgw2.jpg")
    tiff1.Write()
    tiff2.Write()
    bmp1.Write()
    bmp2.Write()
    pnm1.Write()
    pnm2.Write()
    psw1.Write()
    psw2.Write()
    pngw1.Write()
    pngw2.Write()
    jpgw1.Write()
    jpgw2.Write()
    file.delete("-force", "" + str(dir) + "/tiff1.tif")
    file.delete("-force", "" + str(dir) + "/tiff2.tif")
    file.delete("-force", "" + str(dir) + "/bmp1.bmp")
    file.delete("-force", "" + str(dir) + "/bmp2.bmp")
    file.delete("-force", "" + str(dir) + "/pnm1.pnm")
    file.delete("-force", "" + str(dir) + "/pnm2.pnm")
    file.delete("-force", "" + str(dir) + "/psw1.ps")
    file.delete("-force", "" + str(dir) + "/psw2.ps")
    file.delete("-force", "" + str(dir) + "/pngw1.png")
    file.delete("-force", "" + str(dir) + "/pngw2.png")
    file.delete("-force", "" + str(dir) + "/jpgw1.jpg")
    file.delete("-force", "" + str(dir) + "/jpgw2.jpg")
    pass
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(luminance.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
