# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkFiltersCore as ff
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkIOXML as ix
from vtkmodules import vtkIOIOSS as ii
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingOpenGL2 import *
from vtkmodules.vtkRenderingCellGrid import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget

# We need vtkIOExodus as otherwise an information key will be missing, generating
# the following warning:
#   Could not locate key vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE.
from vtkmodules import vtkIOExodus as ie

from vtkmodules.test import Testing
import os

class TestCellGridPointProbe(Testing.vtkTest):

    def testProbe(self):
        cc.vtkLogger.SetStderrVerbosity(cc.vtkLogger.VERBOSITY_TRACE)
        fc.vtkFiltersCellGrid.RegisterCellsAndResponders()
        # reader = ii.vtkIOSSReader()
        # reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'can.ex2'))
        reader = io.vtkCellGridReader()
        reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        reader.UpdateInformation()
        # Transcription does not currently support merging blocks/sets:
        # reader.MergeExodusEntityBlocksOn()
        # reader.GetSideSetSelection().EnableAllArrays()
        # reader.GetNodeSetSelection().EnableAllArrays()
        reader.Update()
        img = dm.vtkImageData()
        img.SetOrigin(0, 0, 0)
        img.SetDimensions(16,  8,  8);
        img.SetSpacing(0.125, 0.125, 0.125)
        cellCenters = ff.vtkCellCenters()
        cellCenters.VertexCellsOn()
        cellCenters.SetInputDataObject(0, img)
        cellCenters.Update()
        pointProbe = fc.vtkCellGridPointProbe()
        pointProbe.SetAttributeName('curl1')
        pointProbe.SetInputConnection(0, cellCenters.GetOutputPort())
        pointProbe.SetInputConnection(1, reader.GetOutputPort())
        pointProbe.Update()
        sampled = pointProbe.GetOutputDataObject(0)
        xw = ix.vtkXMLPolyDataWriter()
        xw.SetInputConnection(0, pointProbe.GetOutputPort())
        xw.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-probed.vtp'))
        xw.Write()

if __name__ == "__main__":
    Testing.main([(TestCellGridPointProbe, 'test')])
