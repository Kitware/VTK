#!/usr/bin/env python

import os
from vtkmodules.vtkCommonDataModel import vtkRectilinearGrid
from vtkmodules.vtkFiltersExtraction import vtkExtractRectilinearGrid
from vtkmodules.vtkIOLegacy import vtkRectilinearGridReader
from vtkmodules.vtkIOXML import (
    vtkXMLRectilinearGridReader,
    vtkXMLRectilinearGridWriter,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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
file0 = VTK_TEMP_DIR + '/rgFile0.vtr'
file1 = VTK_TEMP_DIR + '/rgFile1.vtr'
file2 = VTK_TEMP_DIR + '/rgFile2.vtr'

# read in some grid data
gridReader = vtkRectilinearGridReader()
gridReader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtk")
gridReader.Update()

# extract to reduce extents of grid
extract = vtkExtractRectilinearGrid()
extract.SetInputConnection(gridReader.GetOutputPort())
extract.SetVOI(0, 23, 0, 32, 0, 10)
extract.Update()

# write just a piece (extracted piece) as well as the whole thing
rgWriter = vtkXMLRectilinearGridWriter()
rgWriter.SetFileName(file0)
rgWriter.SetInputConnection(extract.GetOutputPort())
rgWriter.SetDataModeToAscii()
rgWriter.Write()

rgWriter.SetFileName(file1)
rgWriter.SetInputConnection(gridReader.GetOutputPort())
rgWriter.SetDataModeToAppended()
rgWriter.SetNumberOfPieces(2)
rgWriter.Write()

rgWriter.SetFileName(file2)
rgWriter.SetDataModeToBinary()
rgWriter.SetWriteExtent(3, 46, 6, 32, 1, 5)
rgWriter.SetCompressor(None)
if rgWriter.GetByteOrder():
    rgWriter.SetByteOrder(0)
else:
    rgWriter.SetByteOrder(1)
rgWriter.Write()

# read the extracted grid
reader = vtkXMLRectilinearGridReader()
reader.SetFileName(file0)
reader.WholeSlicesOff()
reader.Update()

rg0 = vtkRectilinearGrid()
rg0.DeepCopy(reader.GetOutput())
mapper0 = vtkDataSetMapper()
mapper0.SetInputData(rg0)

actor0 = vtkActor()
actor0.SetMapper(mapper0)

# read the whole grid
reader.SetFileName(file1)
reader.WholeSlicesOn()
reader.Update()

rg1 = vtkRectilinearGrid()
rg1.DeepCopy(reader.GetOutput())
mapper1 = vtkDataSetMapper()
mapper1.SetInputData(rg1)

actor1 = vtkActor()
actor1.SetMapper(mapper1)
actor1.SetPosition(-1.5, 3, 0)

# read the partially written grid
reader.SetFileName(file2)
reader.Update()

mapper2 = vtkDataSetMapper()
mapper2.SetInputConnection(reader.GetOutputPort())

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5, 3, 0)

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

renWin.SetSize(300, 300)
renWin.Render()

os.remove(file0)
os.remove(file1)
os.remove(file2)
