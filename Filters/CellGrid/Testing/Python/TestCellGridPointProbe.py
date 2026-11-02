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
        pointProbe.GetAttributeSelection().EnableArray('curl1')
        pointProbe.SetInputConnection(0, cellCenters.GetOutputPort())
        pointProbe.SetInputConnection(1, reader.GetOutputPort())
        pointProbe.Update()
        sampled = pointProbe.GetOutputDataObject(0)
        xw = ix.vtkXMLPolyDataWriter()
        xw.SetInputConnection(0, pointProbe.GetOutputPort())
        xw.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-probed.vtp'))
        xw.Write()

    def testProbeAllAttributes(self):
        fc.vtkFiltersCellGrid.RegisterCellsAndResponders()
        reader = io.vtkCellGridReader()
        reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        reader.Update()
        cellgrid = reader.GetOutputDataObject(0)
        shapeId = cellgrid.GetShapeAttribute().GetId()
        attributes = [att for att in cellgrid.GetCellAttributeList() if att.GetId() != shapeId]
        # Several attributes are needed to interpolate them from one classification.
        self.assertGreater(len(attributes), 1)

        img = dm.vtkImageData()
        img.SetOrigin(0, 0, 0)
        img.SetDimensions(16,  8,  8)
        img.SetSpacing(0.125, 0.125, 0.125)
        cellCenters = ff.vtkCellCenters()
        cellCenters.VertexCellsOn()
        cellCenters.SetInputDataObject(0, img)

        # Every enabled attribute is interpolated, but never the shape.
        allProbe = fc.vtkCellGridPointProbe()
        allProbe.SetInputConnection(0, cellCenters.GetOutputPort())
        allProbe.SetInputConnection(1, reader.GetOutputPort())
        for attribute in cellgrid.GetCellAttributeList():
            allProbe.GetAttributeSelection().EnableArray(attribute.GetName().Data())
        allProbe.Update()
        allSampled = allProbe.GetOutputDataObject(0)
        allPD = allSampled.GetPointData()
        self.assertGreater(allSampled.GetNumberOfPoints(), 0)
        self.assertIsNone(allPD.GetArray(cellgrid.GetShapeAttribute().GetName().Data()))
        self.assertIsNone(allPD.GetScalars())

        # Each attribute must match probing that attribute alone.
        oneProbe = fc.vtkCellGridPointProbe()
        oneProbe.SetInputConnection(0, cellCenters.GetOutputPort())
        oneProbe.SetInputConnection(1, reader.GetOutputPort())
        for attribute in attributes:
            name = attribute.GetName().Data()
            oneProbe.GetAttributeSelection().DisableAllArrays()
            oneProbe.GetAttributeSelection().EnableArray(name)
            oneProbe.Update()
            oneSampled = oneProbe.GetOutputDataObject(0)
            expected = oneSampled.GetPointData().GetArray(name)
            actual = allPD.GetArray(name)
            self.assertIsNotNone(actual, name)
            self.assertEqual(actual.GetNumberOfComponents(), attribute.GetNumberOfComponents())
            self.assertEqual(actual.GetNumberOfTuples(), oneSampled.GetNumberOfPoints())
            self.assertEqual(actual.GetNumberOfTuples(), allSampled.GetNumberOfPoints())
            for ii in range(actual.GetNumberOfTuples()):
                self.assertEqual(actual.GetTuple(ii), expected.GetTuple(ii), name)

if __name__ == "__main__":
    Testing.main([(TestCellGridPointProbe, 'test')])
