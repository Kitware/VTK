#!/usr/bin/env python

import os
from vtkmodules.vtkCommonDataModel import vtkStructuredGrid
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersExtraction import vtkExtractGrid
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkIOXML import (
    vtkXMLStructuredGridReader,
    vtkXMLStructuredGridWriter,
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

file0 = VTK_TEMP_DIR + '/sgFile0.vts'
file1 = VTK_TEMP_DIR + '/sgFile1.vts'
file2 = VTK_TEMP_DIR + '/sgFile2.vts'

# Create a reader and write out the field
combReader = vtkMultiBlockPLOT3DReader()
combReader.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
combReader.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
combReader.SetScalarFunctionNumber(100)
combReader.Update()
output = combReader.GetOutput().GetBlock(0)


# extract to reduce extents of grid
extract = vtkExtractGrid()
extract.SetInputData(output)
extract.SetVOI(0, 28, 0, 32, 0, 24)
extract.Update()

# write just a piece (extracted piece) as well as the whole thing
gridWriter = vtkXMLStructuredGridWriter()
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
reader = vtkXMLStructuredGridReader()
reader.SetFileName(file0)
reader.WholeSlicesOff()
reader.Update()

sg = vtkStructuredGrid()
sg.DeepCopy(reader.GetOutput())
cF0 = vtkContourFilter()
cF0.SetInputData(sg)
cF0.SetValue(0, 0.38)

mapper0 = vtkPolyDataMapper()
mapper0.SetInputConnection(cF0.GetOutputPort())
mapper0.ScalarVisibilityOff()

actor0 = vtkActor()
actor0.SetMapper(mapper0)


# read the whole image
reader.SetFileName(file1)
reader.WholeSlicesOn()
reader.Update()

sg1 = vtkStructuredGrid()
sg1.DeepCopy(reader.GetOutput())
cF1 = vtkContourFilter()
cF1.SetInputData(sg1)
cF1.SetValue(0, 0.38)

mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(cF1.GetOutputPort())
mapper1.ScalarVisibilityOff()

actor1 = vtkActor()
actor1.SetMapper(mapper1)
actor1.SetPosition(0, -10, 0)


# read the partially written grid
reader.SetFileName(file2)
reader.Update()

cF2 = vtkContourFilter()
cF2.SetInputConnection(reader.GetOutputPort())
cF2.SetValue(0, 0.38)

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(cF2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 10, 0)

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
