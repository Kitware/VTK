#!/usr/bin/env python

# $ vtkpython TestTextureGlyph.py --help
# provides more details on other options.

import os
import os.path
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersSources import vtkCubeSource
from vtkmodules.vtkIOImage import vtkBMPReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

class TestTextureGlyph(Testing.vtkTest):
    def testGlyphs(self):
        """Test if texturing of the glyphs works correctly."""
        # The Glyph
        cs = vtkCubeSource()
        cs.SetXLength(2.0); cs.SetYLength(1.0); cs.SetZLength(0.5)

        # Create input point data.
        pts = vtkPoints()
        pts.InsertPoint(0, (1,1,1))
        pts.InsertPoint(1, (0,0,0))
        pts.InsertPoint(2, (-1,-1,-1))
        polys = vtkCellArray()
        polys.InsertNextCell(1)
        polys.InsertCellPoint(0)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(1)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(2)
        pd = vtkPolyData()
        pd.SetPoints(pts)
        pd.SetPolys(polys)

        # Orient the glyphs as per vectors.
        vec = vtkFloatArray()
        vec.SetNumberOfComponents(3)
        vec.InsertTuple3(0, 1, 0, 0)
        vec.InsertTuple3(1, 0, 1, 0)
        vec.InsertTuple3(2, 0, 0, 1)
        pd.GetPointData().SetVectors(vec)

        # The glyph filter.
        g = vtkGlyph3D()
        g.SetScaleModeToDataScalingOff()
        g.SetVectorModeToUseVector()
        g.SetInputData(pd)
        g.SetSourceConnection(cs.GetOutputPort())

        m = vtkPolyDataMapper()
        m.SetInputConnection(g.GetOutputPort())
        a = vtkActor()
        a.SetMapper(m)

        # The texture.
        img_file = os.path.join(VTK_DATA_ROOT, "Data", "masonry.bmp")
        img_r = vtkBMPReader()
        img_r.SetFileName(img_file)
        t = vtkTexture()
        t.SetInputConnection(img_r.GetOutputPort())
        t.InterpolateOn()
        a.SetTexture(t)

        # Renderer, RenderWindow etc.
        ren = vtkRenderer()
        ren.SetBackground(0.5, 0.5, 0.5)
        ren.AddActor(a)

        ren.ResetCamera();
        cam = ren.GetActiveCamera()
        cam.Azimuth(-90)
        cam.Zoom(1.4)

        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        rwi = vtkRenderWindowInteractor()
        rwi.SetRenderWindow(renWin)
        rwi.Initialize()
        rwi.Render()

if __name__ == "__main__":
    Testing.main([(TestTextureGlyph, 'test')])
