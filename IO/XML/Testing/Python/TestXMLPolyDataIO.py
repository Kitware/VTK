#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

file0 = 'idFile0.vti'
file1 = 'idFile1.vti'
file2 = 'idFile2.vti'

# read in some poly data
pdReader = vtk.vtkPolyDataReader()
pdReader.SetFileName(VTK_DATA_ROOT + "/Data/fran_cut.vtk")
pdReader.Update()

extract = vtk.vtkExtractPolyDataPiece()
extract.SetInputConnection(pdReader.GetOutputPort())

# write various versions
pdWriter = vtk.vtkXMLPolyDataWriter()
pdWriter.SetFileName(file0)
pdWriter.SetDataModeToAscii()
pdWriter.SetInputConnection(pdReader.GetOutputPort())
pdWriter.Write()

pdWriter.SetFileName(file1)
pdWriter.SetInputConnection(extract.GetOutputPort())
pdWriter.SetDataModeToAppended()
pdWriter.SetNumberOfPieces(2)
pdWriter.Write()

pdWriter.SetFileName(file2)
pdWriter.SetDataModeToBinary()
pdWriter.SetGhostLevel(3)
pdWriter.Write()


# read the ASCII version
reader = vtk.vtkXMLPolyDataReader()
reader.SetFileName(file0)
reader.Update()

pd0 = vtk.vtkPolyData()
pd0.DeepCopy(reader.GetOutput())
mapper0 = vtk.vtkPolyDataMapper()
mapper0.SetInputData(pd0)

actor0 = vtk.vtkActor()
actor0.SetMapper(mapper0)
actor0.SetPosition(0, .15, 0)


# read appended piece 0
reader.SetFileName(file1)

mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(reader.GetOutputPort())
mapper1.SetPiece(0)
mapper1.SetNumberOfPieces(2)

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)


# read binary piece 0 (with ghost level)
reader2 = vtk.vtkXMLPolyDataReader()
reader2.SetFileName(file2)

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(reader2.GetOutputPort())
mapper2.SetPiece(0)
mapper2.SetNumberOfPieces(2)

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 0, 0.1)

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

ren.GetActiveCamera().SetPosition(0.514096, -0.14323, -0.441177)
ren.GetActiveCamera().SetFocalPoint(0.0528, -0.0780001, -0.0379661)
renWin.SetSize(300, 300)
renWin.Render()

os.remove(file0)
os.remove(file1)
os.remove(file2)
