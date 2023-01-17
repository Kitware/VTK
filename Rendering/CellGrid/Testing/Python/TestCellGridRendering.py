from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkRenderingCore as rr
import vtkmodules.vtkRenderingCellGrid
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtkmodules.test import Testing
import os

class TestCellGridRendering(Testing.vtkTest):

    def testDGHexRendering(self):
        rh = io.vtkCellGridReader()
        rh.SetFileName(os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        fh = fc.vtkCellGridComputeSurface()
        fh.SetInputConnection(rh.GetOutputPort())
        fh.Update()
        ah = rr.vtkActor()
        mh = rr.vtkCellGridMapper()
        mh.SetInputConnection(fh.GetOutputPort())
        mh.ScalarVisibilityOn()
        mh.SetArrayName('scalar1')
        ah.SetMapper(mh)
        rw = rr.vtkRenderWindow()
        rn = rr.vtkRenderer()
        rw.AddRenderer(rn)
        rn.AddActor(ah)
        rw.SetSize(300, 300)
        rw.Render()
        img_file = "TestCellGridRendering-Hexahedra.png"
        Testing.compareImage(rw, Testing.getAbsImagePath(img_file), threshold=25)
        Testing.interact()

    def testDGTetRendering(self):
        rh = io.vtkCellGridReader()
        rh.SetFileName(os.path.join(VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg'))
        fh = fc.vtkCellGridComputeSurface()
        fh.SetInputConnection(rh.GetOutputPort())
        fh.Update()
        ah = rr.vtkActor()
        mh = rr.vtkCellGridMapper()
        mh.SetInputConnection(fh.GetOutputPort())
        mh.ScalarVisibilityOn()
        mh.SetArrayName('scalar2')
        ah.SetMapper(mh)
        rw = rr.vtkRenderWindow()
        rn = rr.vtkRenderer()
        rw.AddRenderer(rn)
        rn.AddActor(ah)
        rw.SetSize(300, 300)
        rw.Render()
        img_file = "TestCellGridRendering-Tetrahedra.png"
        Testing.compareImage(rw, Testing.getAbsImagePath(img_file), threshold=25)
        Testing.interact()

if __name__ == "__main__":
    Testing.main([(TestCellGridRendering, 'test')])
