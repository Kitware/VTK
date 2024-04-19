#!/usr/bin/env python

import os
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkFiltersParallel import vtkExtractUnstructuredGridPiece
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
from vtkmodules.vtkIOXML import (
    vtkXMLUnstructuredGridReader,
    vtkXMLUnstructuredGridWriter,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

file0 = VTK_TEMP_DIR + '/ugFile0.vtu'
file1 = VTK_TEMP_DIR + '/ugFile1.vtu'
file2 = VTK_TEMP_DIR + '/ugFile2.vtu'

# read in some unstructured grid data
ugReader = vtkUnstructuredGridReader()
ugReader.SetFileName(VTK_DATA_ROOT + "/Data/blow.vtk")
ugReader.SetScalarsName("thickness9")
ugReader.SetVectorsName("displacement9")

extract = vtkExtractUnstructuredGridPiece()
extract.SetInputConnection(ugReader.GetOutputPort())

# write various versions
ugWriter = vtkXMLUnstructuredGridWriter()
ugWriter.SetFileName(file0)
ugWriter.SetDataModeToAscii()
ugWriter.SetInputConnection(ugReader.GetOutputPort())
ugWriter.Write()

ugWriter.SetFileName(file1)
ugWriter.SetInputConnection(extract.GetOutputPort())
ugWriter.SetDataModeToAppended()
ugWriter.SetNumberOfPieces(2)
ugWriter.Write()

ugWriter.SetFileName(file2)
ugWriter.SetDataModeToBinary()
ugWriter.SetGhostLevel(2)
ugWriter.Write()


# read the ASCII version
reader = vtkXMLUnstructuredGridReader()
reader.SetFileName(file0)
reader.Update()

ug0 = vtkUnstructuredGrid()
ug0.DeepCopy(reader.GetOutput())
sF = vtkDataSetSurfaceFilter()
sF.SetInputData(ug0)

mapper0 = vtkPolyDataMapper()
mapper0.SetInputConnection(sF.GetOutputPort())

actor0 = vtkActor()
actor0.SetMapper(mapper0)
actor0.SetPosition(0, 40, 20)


# read appended piece 0
reader.SetFileName(file1)

sF1 = vtkDataSetSurfaceFilter()
sF1.SetInputConnection(reader.GetOutputPort())

mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(sF1.GetOutputPort())
mapper1.SetPiece(1)
mapper1.SetNumberOfPieces(2)

actor1 = vtkActor()
actor1.SetMapper(mapper1)


# read binary piece 0 (with ghost level)
reader2 = vtkXMLUnstructuredGridReader()
reader2.SetFileName(file2)

sF2 = vtkDataSetSurfaceFilter()
sF2.SetInputConnection(reader2.GetOutputPort())

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(sF2.GetOutputPort())
mapper2.SetPiece(1)
mapper2.SetNumberOfPieces(2)
mapper2.SetGhostLevel(2)

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 0, 30)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor0)
ren.AddActor(actor1)
ren.AddActor(actor2)

ren.ResetCamera()
ren.GetActiveCamera().SetPosition(180, 55, 65)
ren.GetActiveCamera().SetFocalPoint(3.5, 32, 15)
renWin.SetSize(300, 300)
renWin.Render()

#os.remove(file0)
#os.remove(file1)
#os.remove(file2)
