# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkFiltersSources as fs
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

class TestCellGridToUnstructuredGrid(Testing.vtkTest):

    def testCellCenters(self):
        """Test that we can glyph HCurl/HDiv vector attributes."""
        bds = [0, 0, 0, 0, 0, 0]

        rdr = io.vtkCellGridReader()
        rdr.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        flagEdgesVerts = dm.vtkCellGridSidesQuery.EdgesOfInputs | dm.vtkCellGridSidesQuery.VerticesOfInputs
        flagEdgesOnly = dm.vtkCellGridSidesQuery.EdgesOfInputs

        srf = (rdr >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesOnly)).last
        sid = (rdr >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesVerts)).last
        pdc = (sid >> fc.vtkCellGridCellCenters() >> fc.vtkCellGridToUnstructuredGrid()).last

        shapeMapper = (sid >> vtkCellGridMapper(scalar_visibility=False)).last
        sppty = vtkProperty(opacity=1., line_width=1, point_size=8, color=(0,0,0))
        shapeActor = vtkActor(mapper=shapeMapper, property=sppty)

        pdc.Update()
        arrowSource = fs.vtkArrowSource(
            tip_resolution=64,
            shaft_resolution=64,
        )
        glyphMapper = (pdc >> vtkGlyph3DMapper(
            array_component=-1,
            orient=True,
            orientation_array='curl1',
            source_connection=arrowSource.GetOutputPort(),
            scaling=True,
            scale_mode=vtkGlyph3DMapper.SCALE_BY_MAGNITUDE,
            scale_array='curl1',
            scale_factor=0.25,
        )).last
        glyphMapper.SelectColorArray('scalar1')

        cppty = vtkProperty(line_width=5, point_size=4, color=(0.5,0.5,0.75))
        glyphActor = vtkActor(mapper=glyphMapper, property=cppty)

        ren = vtkRenderer()
        ren.AddActor(shapeActor)
        ren.AddActor(glyphActor)

        rw = vtkRenderWindow()
        rw.AddRenderer(ren)
        ren.SetBackground(1.0, 1.0, 1.0)

        rwi = rw.MakeRenderWindowInteractor()
        rwi.SetRenderWindow(rw)

        cow = vtkCameraOrientationWidget()
        cow.SetParentRenderer(ren)
        cow.On()

        sid.GetOutputDataObject(0).GetBounds(bds)
        cam = ren.GetActiveCamera()
        rw.Render()
        ren.ResetCamera()
        cam.Zoom(2.0)

        if '-I' in sys.argv:
            wri = io.vtkCellGridWriter()
            wri.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-to-unstructured.dg'))
            wri.SetInputConnection(sid.GetOutputPort())
            wri.Write()

            wru = ix.vtkXMLUnstructuredGridWriter()
            wru.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-to-unstructured.vtu'))
            wru.SetDataModeToAscii()
            wru.SetInputConnection(pdc.GetOutputPort())
            wru.Write()

        rwi.Initialize()
        rw.Render()
        if '-I' in sys.argv:
            rwi.Start()
            print('camera', cam)
        baseline = 'TestCellGridToUnstructuredGrid-Centers.png'
        Testing.compareImage(rw, Testing.getAbsImagePath(baseline))

    def testMultipleCellTypes(self):
        bds = [0, 0, 0, 0, 0, 0]

        rdr = io.vtkCellGridReader()
        rdr.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgMixed.dg'))

        srf = fc.vtkCellGridComputeSides()
        srf.SetInputConnection(rdr.GetOutputPort())
        sid = fc.vtkCellGridComputeSides()
        sid.SetOutputDimensionControl(dm.vtkCellGridSidesQuery.EdgesOfInputs)
        # sid.OmitSidesForRenderableInputsOff()
        sid.SetInputConnection(srf.GetOutputPort())
        sid.Update()

        pdc = fc.vtkCellGridToUnstructuredGrid()
        pdc.SetInputConnection(rdr.GetOutputPort())

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

        pdc.Update()
        dataMapper = vtkDataSetMapper()
        dataMapper.SetInputConnection(pdc.GetOutputPort())
        dataMapper.ScalarVisibilityOn()
        dataMapper.SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
        dataMapper.SelectColorArray('scalar1')
        dataMapper.SetArrayComponent(0)
        lkup = cc.vtkLookupTable()
        clr = cl.vtkColorSeries(color_scheme=cl.vtkColorSeries.BREWER_QUALITATIVE_DARK2)
        clr.BuildLookupTable(lkup, cl.vtkColorSeries.ORDINAL);
        dataMapper.SetLookupTable(lkup)

        dataActor = vtkActor()
        dataActor.SetMapper(dataMapper)
        cppty = vtkProperty()
        cppty.SetOpacity(0.7)
        cppty.SetLineWidth(5)
        cppty.SetPointSize(8)
        cppty.SetColor(1, 0, 0)
        dataActor.SetProperty(cppty)

        ren = vtkRenderer()
        ren.AddActor(shapeActor)
        ren.AddActor(dataActor)

        rw = vtkRenderWindow()
        rw.AddRenderer(ren)
        ren.SetBackground(1.0, 1.0, 1.0)

        rwi = rw.MakeRenderWindowInteractor()
        rwi.SetRenderWindow(rw)

        cow = vtkCameraOrientationWidget()
        cow.SetParentRenderer(ren)
        cow.On()

        sid.GetOutputDataObject(0).GetBounds(bds)
        cam = ren.GetActiveCamera()

        cam.SetViewUp(0., 0., -1.)
        cam.SetPosition(1.21346, 1.24257, 5.66879)
        cam.SetFocalPoint(0.146239, 0.394899, 0.0458881)
        cam.SetDistance(4.63531)

        if '-I' in sys.argv:
            wri = io.vtkCellGridWriter()
            wri.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-to-unstructured.dg'))
            wri.SetInputConnection(sid.GetOutputPort())
            wri.Write()

            wru = ix.vtkXMLUnstructuredGridWriter()
            wru.SetFileName(os.path.join(Testing.VTK_TEMP_DIR, 'cell-grid-to-unstructured.vtu'))
            wru.SetDataModeToAscii()
            wru.SetInputConnection(pdc.GetOutputPort())
            wru.Write()

        rwi.Initialize()
        rw.Render()
        if '-I' in sys.argv:
            rwi.Start()
            print('camera', cam)
        baseline = 'TestCellGridToUnstructuredGrid-Mixed.png'
        Testing.compareImage(rw, Testing.getAbsImagePath(baseline))

if __name__ == "__main__":
    Testing.main([(TestCellGridToUnstructuredGrid, 'test')])
