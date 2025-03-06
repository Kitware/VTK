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
from vtkmodules import vtkCommonColor as cl
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingOpenGL2 import *
from vtkmodules.vtkRenderingCellGrid import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget

from vtkmodules.test import Testing
import os
import sys

# Register render responder for DG cells:
vtkRenderingCellGrid.RegisterCellsAndResponders()
fc.vtkFiltersCellGrid.RegisterCellsAndResponders()

class TestCellGridCellCenters(Testing.vtkTest):

    def runCase(self, filename, baseline, options):
        bds = [0, 0, 0, 0, 0, 0]

        rdr = io.vtkCellGridReader()
        rdr.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', filename))

        srf = fc.vtkCellGridComputeSides()
        srf.SetInputConnection(rdr.GetOutputPort())
        sid = fc.vtkCellGridComputeSides()
        sid.SetOutputDimensionControl(dm.vtkCellGridSidesQuery.EdgesOfInputs)
        sid.OmitSidesForRenderableInputsOff()
        sid.SetInputConnection(srf.GetOutputPort())
        sid.Update()

        ctr = fc.vtkCellGridCellCenters()
        cport = rdr.GetOutputPort() if 'test-sides' not in options else sid.GetOutputPort()
        ctr.SetInputConnection(cport)

        if 'test-sides' in options:
            sd2 = (rdr >> fc.vtkCellGridComputeSides(
                output_dimension_control=options['test-sides'],
                omit_sides_for_renderable_inputs=False)).last
            ctr.SetInputConnection(sd2.GetOutputPort())

        pdc = fc.vtkCellGridToUnstructuredGrid()
        pdc.SetInputConnection(ctr.GetOutputPort())

        shapeMapper = vtkCellGridMapper()
        shapeMapper.SetInputConnection(sid.GetOutputPort())
        shapeMapper.ScalarVisibilityOff()
        shapeActor = vtkActor()
        shapeActor.SetMapper(shapeMapper)
        sppty = vtkProperty()
        sppty.SetOpacity(1.0)
        sppty.SetLineWidth(1)
        sppty.SetPointSize(8)
        sppty.SetColor(0, 0, 0)
        shapeActor.SetProperty(sppty)

        ctr.Update()
        pdc.Update()
        centerMapper = vtkCellGridMapper()
        centerMapper.SetInputConnection(ctr.GetOutputPort())
        centerMapper.ScalarVisibilityOff()
        centerActor = vtkActor()
        centerActor.SetMapper(centerMapper)
        cppty = vtkProperty()
        cppty.SetOpacity(1.0)
        cppty.SetLineWidth(5)
        cppty.SetPointSize(8)
        cppty.SetColor(1, 0, 0)
        centerActor.SetProperty(cppty)

        ren = vtkRenderer()
        ren.AddActor(shapeActor)
        ren.AddActor(centerActor)

        rw = vtkRenderWindow()
        rw.SetMultiSamples(0)  # when rendering lines ensure same output since anti-aliasing is not supported on all machines
        rw.AddRenderer(ren)
        ren.SetBackground(1.0, 1.0, 1.0)
        #ren.SetBackground(0.5, 0.4, 0.3)
        #ren.SetBackground2(0.7, 0.6, 0.5)
        #ren.GradientBackgroundOn()

        rwi = rw.MakeRenderWindowInteractor()# vtkRenderWindowInteractor()
        rwi.SetRenderWindow(rw)

        cow = vtkCameraOrientationWidget()
        cow.SetParentRenderer(ren)
        # Enable the widget.
        cow.On()

        sid.GetOutputDataObject(0).GetBounds(bds)
        cam = ren.GetActiveCamera()
        # If you want to change the camera location, this is a good starting point:
        # dc = ((bds[0]+bds[1])/2, (bds[2]+bds[3])/2, (bds[4] + bds[5])/2)
        # cam.SetFocalPoint(dc[0], dc[1], dc[2] + 1)
        # cam.SetPosition(dc[0]+1, dc[1]+1, dc[2]-1.)
        cam.SetViewUp(*options['camera-vup'])
        cam.SetPosition(*options['camera-eye'])
        cam.SetFocalPoint(*options['camera-aim'])
        cam.SetDistance(options['camera-dst'])

        if '-I' in sys.argv:
            wri = io.vtkCellGridWriter()
            wri.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-cell-centers.dg'))
            wri.SetInputConnection(ctr.GetOutputPort())
            wri.Write()

            wru = ix.vtkXMLUnstructuredGridWriter()
            wru.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-cell-centers.vtu'))
            wru.SetDataModeToAscii()
            wru.SetInputConnection(pdc.GetOutputPort())
            wru.Write()

        rwi.Initialize()
        rw.Render()
        print('baseline', baseline, Testing.getAbsImagePath(baseline))
        if '-I' in sys.argv:
            rwi.Start()
            print('camera', cam)
        Testing.compareImage(rw, Testing.getAbsImagePath(baseline))

    def testEdgeCenters(self):
        """Test that the cell-center filter properly computes centers
        of sides, not just the centers of cells."""
        options = {
            'test-sides': dm.vtkCellGridSidesQuery.EdgesOfInputs,
            'camera-eye': (5, 2.5, 2.25),
            'camera-aim': (2, 1, 1),
            'camera-vup': (0, 1, 0),
            'camera-dst': 7 }
        self.runCase('dgHexahedra.dg', 'TestCellGridCellCenters-EdgeSides.png', options)

    def testAllSideCenters(self):
        """Test that the cell-center filter properly computes centers
        of sides, not just the centers of cells."""
        options = {
            'test-sides': dm.vtkCellGridSidesQuery.AllSides,
            'camera-eye': (5, 2.5, 2.25),
            'camera-aim': (2, 1, 1),
            'camera-vup': (0, 1, 0),
            'camera-dst': 7 }
        self.runCase('dgHexahedra.dg', 'TestCellGridCellCenters-AllSides.png', options)

    def testCellCenters(self):
        """Test that the cell-center filter properly computes centers
        of sides, not just the centers of cells."""
        options = {
            'camera-eye': (2.21346, 2.24257, -3.66879),
            'camera-aim': (0.146239, 0.394899, 0.0458881),
            'camera-vup': (0, 0, -1),
            'camera-dst': 4.63531 }
        self.runCase('fandisk.dg', 'TestCellGridCellCenters-Cells.png', options)

if __name__ == "__main__":
    Testing.main([(TestCellGridCellCenters, 'test')])
