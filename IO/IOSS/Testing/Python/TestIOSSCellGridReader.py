# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
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
import sys

# Register render responder for DG cells:
vtkRenderingCellGrid.RegisterCellsAndResponders()
fc.vtkFiltersCellGrid.RegisterCellsAndResponders()

class TestIOSSCellGridReader(Testing.vtkTest):

    def loadAndRender(self, filename, fieldname, time, baseline, expectedUGridCells, expectedCGridCells, expectedBounds):
        #cc.vtkLogger.SetStderrVerbosity(cc.vtkLogger.VERBOSITY_TRACE)
        reader = ii.vtkIOSSCellGridReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        # Transcription does not currently support merging blocks/sets:
        # reader.MergeExodusEntityBlocksOn()
        #
        # reader.GetSideSetSelection().EnableAllArrays()
        # reader.GetNodeSetSelection().EnableAllArrays()
        reader.Update()
        outInfo = reader.GetExecutive().GetOutputInformation(0)
        outInfo.Set(em.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), time);
        ugrid = reader.GetOutputDataObject(0)
        numCells = ugrid.GetNumberOfElements(dm.vtkDataObject.CELL)
        self.assertEqual(ugrid.GetNumberOfElements(dm.vtkDataObject.CELL), expectedUGridCells)
        cgrid = reader.GetOutputDataObject(0)
        self.assertEqual(cgrid.GetNumberOfCells(), expectedCGridCells)
        bds = [0, 0, 0, 0, 0, 0]
        cgrid.GetBounds(bds)
        print('Bounds are ', bds)
        self.assertEqual(bds, expectedBounds)
        sid = fc.vtkCellGridComputeSides()
        sid.PreserveRenderableInputsOn()
        sid.OmitSidesForRenderableInputsOff()
        sid.SetInputConnection(reader.GetOutputPort())
        sid.Update()
        dd = sid.GetOutputDataObject(0)
        # print('Data Assembly\n', dd.GetDataAssembly())
        mapper = vtkCompositeCellGridMapper()
        cdda = vtkCompositeDataDisplayAttributes()
        mapper.SetCompositeDataDisplayAttributes(cdda)
        #cdda.SetBlockVisibility(dd.GetPartitionedDataSet(0).GetPartitionAsDataObject(0), False)
        cdda.SetBlockVisibility(dd.GetPartitionedDataSet(1).GetPartitionAsDataObject(0), False)
        cdda.SetBlockVisibility(dd.GetPartitionedDataSet(2).GetPartitionAsDataObject(0), False)
        cdda.SetBlockVisibility(dd.GetPartitionedDataSet(3).GetPartitionAsDataObject(0), False)
        # cdda.SetBlockVisibility(dd.GetPartitionedDataSet(4).GetPartitionAsDataObject(0), False)
        mapper.SetInputConnection(sid.GetOutputPort())
        mapper.ScalarVisibilityOn()
        mapper.ColorByArrayComponent(fieldname, 0)
        mapper.SetScalarModeToUseCellFieldData()
        # cansurf = dd.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        # print(cansurf)

        actor = vtkActor()
        actor.SetMapper(mapper)
        ppty = vtkProperty()
        ppty.SetOpacity(1.0)
        ppty.SetLineWidth(5)
        ppty.SetPointSize(8)
        actor.SetProperty(ppty)

        ren = vtkRenderer()
        ren.AddActor(actor)

        rw = vtkRenderWindow()
        rw.AddRenderer(ren)
        # ren.SetBackground(0.1, 0.1, 0.1)
        ren.SetBackground(0.5, 0.4, 0.3)
        ren.SetBackground2(0.7, 0.6, 0.5)
        ren.GradientBackgroundOn()

        rwi = rw.MakeRenderWindowInteractor()# vtkRenderWindowInteractor()
        rwi.SetRenderWindow(rw)

        cow = vtkCameraOrientationWidget()
        cow.SetParentRenderer(ren)
        # Enable the widget.
        cow.On()

        # Orient the camera to look along -Y
        ctr = [(bds[0]+bds[1])/2.0, (bds[2] + bds[3])/2.0, (bds[4]+bds[5])/2.0]
        pos = [ctr[0], bds[2], ctr[2]]
        cam = ren.GetActiveCamera()
        cam.SetFocalPoint(*ctr)
        cam.SetPosition(*pos)
        cam.SetViewUp(0., 0., -1.)
        ren.ResetCamera()
        rwi.Initialize()
        rw.Render()
        rw.Render()
        # rwi.Start()
        if '-I' in sys.argv:
            rwi.Start()
        Testing.compareImage(rw, Testing.getAbsImagePath(baseline))
        # Testing.interact()

    def testCellData(self):
        filename = os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'can.exdg')
        # Bounds @ t0
        bds = [ \
            -7.878461837768555, 8.312582015991211, \
             0.0, 8.0, \
             -14.999999046325684, 4.778104782104492]
        # Bounds @ t0.001
        # bds = [ \
        #     -7.857451429590583, 8.363625898957253, \
        #     -0.5560619235038757, 8.032422546297312, \
        #     -14.999999046325684, -0.012999534606933594]
        # Bounds @ t1.0
        # bds = [ \
        #     -12.16960620880127, 9.088453590869904, \
        #     -3.600424289703369, 8.433861315250397, \
        #     -19.3181009888649, -11.692354202270508]
        self.loadAndRender(filename, 'EQPS', 0.001, 'TestIOSSCellGridReader-DG-C0.png', 7152, 7152, bds)

    def testPointData(self):
        filename = os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'disk_out_ref.exdg')
        bds = [-5.75, 5.75, -5.75, 5.75, -10.0, 10.15999984741211]
        # Note the expected number of cells of the ioss reader output and the
        # cell-grid differ by 2 because we do not transcribe node sets:
        self.loadAndRender(filename, 'AsH3', 0.0, 'TestIOSSCellGridReader-CG-C1.png', 7472, 7472, bds)

if __name__ == "__main__":
    Testing.main([(TestIOSSCellGridReader, 'test')])
