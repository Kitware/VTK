# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonTransforms as ct
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

class TestCellGridTransform(Testing.vtkTest):

    def runCase(self, attributeName, baseline):
        """Test that we properly transform HCurl/HDiv vector attributes."""
        bds = [0, 0, 0, 0, 0, 0]

        rdr = io.vtkCellGridReader()
        rdr.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        flagEdgesVerts = dm.vtkCellGridSidesQuery.EdgesOfInputs | dm.vtkCellGridSidesQuery.VerticesOfInputs
        flagEdgesOnly = dm.vtkCellGridSidesQuery.EdgesOfInputs
        sf2 = (rdr >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesOnly)).last

        xfm = ct.vtkTransform()
        xfm.RotateZ(-60)
        tfm = (rdr >> fc.vtkCellGridTransform(transform=xfm)).last
        srf = (tfm >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesOnly)).last
        # Swap this out once the range-responder handles sides:
        # sid = (tfm >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesVerts)).last
        sid = (tfm >> fc.vtkCellGridComputeSides(output_dimension_control=flagEdgesOnly)).last
        pdc = (sid >> fc.vtkCellGridCellCenters() >> fc.vtkCellGridToUnstructuredGrid()).last

        shapeMapper2 = (sf2 >> vtkCellGridMapper(scalar_visibility=False)).last
        sppty2 = vtkProperty(opacity=1., line_width=1, point_size=8, color=(0,0,0))
        shapeActor2 = vtkActor(mapper=shapeMapper2, property=sppty2)

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
            orientation_array=attributeName,
            source_connection=arrowSource.GetOutputPort(),
            scaling=True,
            scale_mode=vtkGlyph3DMapper.SCALE_BY_MAGNITUDE,
            scale_array=attributeName,
            scale_factor=0.25,
        )).last
        glyphMapper.SelectColorArray('scalar1')

        cppty = vtkProperty(line_width=5, point_size=4, color=(0.5,0.5,0.75))
        glyphActor = vtkActor(mapper=glyphMapper, property=cppty)

        ren = vtkRenderer()
        ren.AddActor(shapeActor)
        ren.AddActor(shapeActor2)
        ren.AddActor(glyphActor)

        rw = vtkRenderWindow()
        rw.SetMultiSamples(0)  # when rendering lines ensure same output since anti-aliasing is not supported on all machines
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

        def onKeyPress(caller, eventId):
            # print('caller', caller, 'event', eventId)
            key = caller.GetKeySym()
            print('key is "' + key + '"')
            if key == 'plus':
                xfm.RotateZ(5)
                tfm.Modified()
                rw.Render()
            elif key == 'minus':
                xfm.RotateZ(-5)
                tfm.Modified()
                rw.Render()

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
            rwi.AddObserver(cc.vtkCommand.KeyPressEvent, onKeyPress)
            rwi.Start()
            print('camera', cam)
        Testing.compareImage(rw, Testing.getAbsImagePath(baseline))

    def testHCurlVectors(self):
        self.runCase('curl1', 'TestCellGridTransform-HCurl.png')

    def testHDivVectors(self):
        self.runCase('div1', 'TestCellGridTransform-HDiv.png')

if __name__ == "__main__":
    Testing.main([(TestCellGridTransform, 'test')])
