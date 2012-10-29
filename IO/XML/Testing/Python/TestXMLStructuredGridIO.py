#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

file0 = 'sgFile0.vts'
file1 = 'sgFile1.vts'
file2 = 'sgFile2.vts'

# Create a reader and write out the field
combReader = vtk.vtkMultiBlockPLOT3DReader()
combReader.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
combReader.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
combReader.SetScalarFunctionNumber(100)
combReader.Update()
output = combReader.GetOutput().GetBlock(0)


# extract to reduce extents of grid
extract = vtk.vtkExtractGrid()
extract.SetInputData(output)
extract.SetVOI(0, 28, 0, 32, 0, 24)
extract.Update()

# write just a piece (extracted piece) as well as the whole thing
gridWriter = vtk.vtkXMLStructuredGridWriter()
gridWriter.SetFileName(file0)
gridWriter.SetInputConnection(extract.GetOutputPort())
gridWriter.SetDataModeToAscii()
gridWriter.Write()

gridWriter.SetInputData(output)
gridWriter.SetFileName(file1)
gridWriter.SetDataModeToAppended()
gridWriter.SetNumberOfPieces(2)
gridWriter.Write()

gridWriter.SetFileName(file2)
gridWriter.SetDataModeToBinary()
gridWriter.SetWriteExtent(8, 56, 4, 16, 1, 24)
gridWriter.Write()

# read the extracted grid
reader = vtk.vtkXMLStructuredGridReader()
reader.SetFileName(file0)
reader.WholeSlicesOff()
reader.Update()

sg = vtk.vtkStructuredGrid()
sg.DeepCopy(reader.GetOutput())
cF0 = vtk.vtkContourFilter()
cF0.SetInputData(sg)
cF0.SetValue(0, 0.38)

mapper0 = vtk.vtkPolyDataMapper()
mapper0.SetInputConnection(cF0.GetOutputPort())
mapper0.ScalarVisibilityOff()

actor0 = vtk.vtkActor()
actor0.SetMapper(mapper0)


# read the whole image
reader.SetFileName(file1)
reader.WholeSlicesOn()
reader.Update()

sg1 = vtk.vtkStructuredGrid()
sg1.DeepCopy(reader.GetOutput())
cF1 = vtk.vtkContourFilter()
cF1.SetInputData(sg1)
cF1.SetValue(0, 0.38)

mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(cF1.GetOutputPort())
mapper1.ScalarVisibilityOff()

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)
actor1.SetPosition(0, -10, 0)


# read the partially written grid
reader.SetFileName(file2)
reader.Update()

cF2 = vtk.vtkContourFilter()
cF2.SetInputConnection(reader.GetOutputPort())
cF2.SetValue(0, 0.38)

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(cF2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 10, 0)

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
