#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


file0 = 'idFile0.vti'
file1 = 'idFile1.vti'
file2 = 'idFile2.vti'

# read in some image data
imageReader = vtk.vtkImageReader()
imageReader.SetDataByteOrderToLittleEndian()
imageReader.SetDataExtent(0, 63, 0, 63, 1, 93)
imageReader.SetDataSpacing(3.2, 3.2, 1.5)
imageReader.SetFilePrefix(VTK_DATA_ROOT + '/Data/headsq/quarter')
imageReader.Update()

# extract to reduce extents of grid
extract = vtk.vtkExtractVOI()
extract.SetInputConnection(imageReader.GetOutputPort())
extract.SetVOI(0, 63, 0, 63, 0, 45)
extract.Update()

# write just a piece (extracted piece) as well as the whole thing
idWriter = vtk.vtkXMLImageDataWriter()
idWriter.SetFileName(file0)
idWriter.SetDataModeToAscii()
idWriter.SetInputConnection(extract.GetOutputPort())
idWriter.Write()

idWriter.SetFileName(file1)
idWriter.SetDataModeToAppended()
idWriter.SetInputConnection(imageReader.GetOutputPort())
idWriter.SetNumberOfPieces(2)
idWriter.Write()

idWriter.SetFileName(file2)
idWriter.SetDataModeToBinary()
idWriter.SetWriteExtent(1, 31, 4, 63, 12, 92)
idWriter.Write()

# read the extracted grid
reader = vtk.vtkXMLImageDataReader()
reader.SetFileName(file0)
reader.WholeSlicesOff()
reader.Update()

id0 = vtk.vtkImageData()
id0.DeepCopy(reader.GetOutput())
cF0 = vtk.vtkContourFilter()
cF0.SetInputData(id0)
cF0.SetValue(0, 500)

mapper0 = vtk.vtkPolyDataMapper()
mapper0.SetInputConnection(cF0.GetOutputPort())
mapper0.ScalarVisibilityOff()

actor0 = vtk.vtkActor()
actor0.SetMapper(mapper0)
actor0.SetPosition(180, -60, 0)


# read the whole image
reader.SetFileName(file1)
reader.WholeSlicesOn()
reader.Update()

id1 = vtk.vtkImageData()
id1.DeepCopy(reader.GetOutput())
cF1 = vtk.vtkContourFilter()
cF1.SetInputData(id1)
cF1.SetValue(0, 500)

mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(cF1.GetOutputPort())
mapper1.ScalarVisibilityOff()

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)
actor1.SetOrientation(90, 0, 0)


# read the partially written image
reader.SetFileName(file2)
reader.Update()

cF2 = vtk.vtkContourFilter()
cF2.SetInputConnection(reader.GetOutputPort())
cF2.SetValue(0, 500)

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(cF2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.SetOrientation(0, -90, 0)
actor2.SetPosition(180, -30, 0)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor0)
ren.AddActor(actor1)
ren.AddActor(actor2)

renWin.SetSize(300, 300)
renWin.Render()

os.remove(file0)
os.remove(file1)
os.remove(file2)
