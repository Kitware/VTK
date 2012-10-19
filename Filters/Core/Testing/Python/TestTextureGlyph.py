#!/usr/bin/env python

# Run this test like so:
# $ vtkpython TestTextureGlyph.py  -D $VTK_DATA_ROOT \
#   -B $VTK_DATA_ROOT/Baseline/Graphics/
#
# $ vtkpython TestTextureGlyph.py --help
# provides more details on other options.

import os
import os.path
import vtk
from vtk.test import Testing

class TestTextureGlyph(Testing.vtkTest):
    def testGlyphs(self):
        """Test if texturing of the glyphs works correctly."""
        # The Glyph
        cs = vtk.vtkCubeSource()
        cs.SetXLength(2.0); cs.SetYLength(1.0); cs.SetZLength(0.5)

        # Create input point data.
        pts = vtk.vtkPoints()
        pts.InsertPoint(0, (1,1,1))
        pts.InsertPoint(1, (0,0,0))
        pts.InsertPoint(2, (-1,-1,-1))
        polys = vtk.vtkCellArray()
        polys.InsertNextCell(1)
        polys.InsertCellPoint(0)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(1)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(2)
        pd = vtk.vtkPolyData()
        pd.SetPoints(pts)
        pd.SetPolys(polys)

        # Orient the glyphs as per vectors.
        vec = vtk.vtkFloatArray()
        vec.SetNumberOfComponents(3)
        vec.InsertTuple3(0, 1, 0, 0)
        vec.InsertTuple3(1, 0, 1, 0)
        vec.InsertTuple3(2, 0, 0, 1)
        pd.GetPointData().SetVectors(vec)

        # The glyph filter.
        g = vtk.vtkGlyph3D()
        g.SetScaleModeToDataScalingOff()
        g.SetVectorModeToUseVector()
        g.SetInputData(pd)
        g.SetSourceConnection(cs.GetOutputPort())

        m = vtk.vtkPolyDataMapper()
        m.SetInputConnection(g.GetOutputPort())
        a = vtk.vtkActor()
        a.SetMapper(m)

        # The texture.
        img_file = os.path.join(Testing.VTK_DATA_ROOT, "Data",
                                "masonry.bmp")
        img_r = vtk.vtkBMPReader()
        img_r.SetFileName(img_file)
        t = vtk.vtkTexture()
        t.SetInputConnection(img_r.GetOutputPort())
        t.InterpolateOn()
        a.SetTexture(t)

        # Renderer, RenderWindow etc.
        ren = vtk.vtkRenderer()
        ren.SetBackground(0.5, 0.5, 0.5)
        ren.AddActor(a)

        ren.ResetCamera();
        cam = ren.GetActiveCamera()
        cam.Azimuth(-90)
        cam.Zoom(1.4)

        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        rwi = vtk.vtkRenderWindowInteractor()
        rwi.SetRenderWindow(renWin)
        rwi.Initialize()
        rwi.Render()

if __name__ == "__main__":
    Testing.main([(TestTextureGlyph, 'test')])

