# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkInteractionStyle as ii
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkRenderingCore as rr
from vtkmodules import vtkRenderingCellGrid as rg
from vtkmodules import vtkInteractionWidgets as iw

import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtkmodules.test import Testing
import os
import sys

# Register render responder for DG cells:
rg.vtkRenderingCellGrid.RegisterCellsAndResponders()

class TestCellGridRendering(Testing.vtkTest):

    def runCase(self, dataFile, colorArray, imageFile, cell2D = False, angles = (0, 0, 0)):
        rh = io.vtkCellGridReader()
        rh.SetFileName(dataFile)
        fh = fc.vtkCellGridComputeSurface()
        fh.SetInputConnection(rh.GetOutputPort())
        fh.Update()
        # Note: For 2-d cells, we create 2 actor/mapper pairs:
        # + one (ai, mi) to show the original cells
        # + one (ah, mh) to show the edge-sides of the surface
        #   manifold (only those edge-sides on the boundary of the mesh).
        # We use separate mappers so the edges can be rendered without
        # scalar coloring.
        if cell2D:
            ah = rr.vtkActor()
            mh = rr.vtkCellGridMapper()
            mh.SetInputConnection(fh.GetOutputPort())
            ah.SetMapper(mh)
            ah.GetProperty().SetLineWidth(4.0)
        ai = rr.vtkActor()
        mi = rr.vtkCellGridMapper()
        if cell2D:
            mi.SetInputConnection(rh.GetOutputPort())
        else:
            mi.SetInputConnection(fh.GetOutputPort())
        if colorArray != None:
            mi.ScalarVisibilityOn()
            mi.SetScalarMode(rr.VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
            mi.SetArrayName(colorArray)
        ai.SetMapper(mi)
        rw = rr.vtkRenderWindow()
        rn = rr.vtkRenderer()
        ri = rw.MakeRenderWindowInteractor()
        ow = iw.vtkCameraOrientationWidget()
        rw.SetInteractor(ri)
        rw.AddRenderer(rn)
        rn.AddActor(ai)
        ow.SetParentRenderer(rn)
        ow.On()
        if cell2D:
            rn.AddActor(ah)
        rn.GetActiveCamera().Roll(angles[0])
        rn.GetActiveCamera().Azimuth(angles[1])
        rn.GetActiveCamera().Elevation(angles[2])
        rn.ResetCamera()
        rw.SetSize(300, 300)
        rw.Render()
        Testing.compareImage(rw, Testing.getAbsImagePath(imageFile), threshold=25)
        if "-I" in sys.argv:
            rs = ii.vtkInteractorStyleSwitch()
            rs.SetCurrentStyleToMultiTouchCamera()
            ri.SetInteractorStyle(rs)
            rw.Render()
            ri.Start()

    def testDGWdgRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgWedges.dg')
        # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Wedges.png'
        self.runCase(dataFile, 'scalar1', testFile, False, angles=(15, -30, 10))

    def testDGPyrRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgPyramids.dg')
        # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Pyramids.png'
        self.runCase(dataFile, 'scalar1', testFile, False, angles=(-15, 20, -60))
        # Run once coloring by a solid color:
        testFile = 'TestCellGridRendering-Pyramids-uncolored.png'
        self.runCase(dataFile, None, testFile, False)

    def testDGHexRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        # Disabled until the vertex shader is fixed.
        # # Run with HCurl coloring turned on:
        # testFile = 'TestCellGridRendering-Hexahedra-curl1.png'
        # self.runCase(dataFile, 'curl1', testFile, False, angles=(10, 20, 30))
        # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Hexahedra.png'
        self.runCase(dataFile, 'scalar1', testFile, False, angles=(10, 20, 30))
        # Run once coloring by a solid color:
        testFile = 'TestCellGridRendering-Hexahedra-uncolored.png'
        self.runCase(dataFile, None, testFile, False)
        # Run once with quadratic cell coloring turned on:
        testFile = 'TestCellGridRendering-Hexahedra-quadratic.png'
        self.runCase(dataFile, 'quadratic', testFile, False, angles=(10, 20, 30))

    def testDGTetRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg')
        testFile = 'TestCellGridRendering-Tetrahedra.png'
        self.runCase(dataFile, 'scalar2', testFile, False, angles=(10, 20, 20))

    def testDGQuadRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgQuadrilateral.dg')
        testFile = 'TestCellGridRendering-Quadrilateral.png'
        self.runCase(dataFile, 'scalar1', testFile, True, angles=(10, 20, 30))

    def testDGTriRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTriangle.dg')
        testFile = 'TestCellGridRendering-Triangles.png'
        self.runCase(dataFile, 'scalar1', testFile, True, angles=(10, 5, -30))

if __name__ == "__main__":
    Testing.main([(TestCellGridRendering, 'test')])
