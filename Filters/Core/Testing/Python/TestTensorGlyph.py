#!/usr/bin/env python

# Run this test like so:
# vtkpython TestTensorGlyph.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Graphics/

import os
import vtk
from vtk.test import Testing

class SimpleGlyph:
    """A simple class used to test vtkTensorGlyph."""
    def __init__(self, reader):
        self.reader = reader

        sg = self.src_glyph = vtk.vtkSphereSource()
        sg.SetRadius(0.5)
        sg.SetCenter(0.5, 0.0, 0.0)

        g = self.glyph = vtk.vtkTensorGlyph()
        g.SetInputConnection(self.reader.GetOutputPort())
        g.SetSourceConnection(self.src_glyph.GetOutputPort())
        g.SetScaleFactor(0.25)

        # The normals are needed to generate the right colors and if
        # not used some of the glyphs are black.
        self.normals = vtk.vtkPolyDataNormals()
        self.normals.SetInputConnection(g.GetOutputPort())

        self.map = vtk.vtkPolyDataMapper()
        self.map.SetInputConnection(self.normals.GetOutputPort())

        self.act = vtk.vtkActor()
        self.act.SetMapper(self.map)

        # An outline.
        self.of = vtk.vtkOutlineFilter()
        self.of.SetInputConnection(self.reader.GetOutputPort())

        self.out_map = vtk.vtkPolyDataMapper()
        self.out_map.SetInputConnection(self.of.GetOutputPort())

        self.out_act = vtk.vtkActor()
        self.out_act.SetMapper(self.out_map)

    def GetActors(self):
        return self.act, self.out_act

    def Update(self):
        self.glyph.Update()

        s = self.glyph.GetOutput().GetPointData().GetScalars()
        if s:
            self.map.SetScalarRange(s.GetRange())

    def SetPosition(self, pos):
        self.act.SetPosition(pos)
        self.out_act.SetPosition(pos)


class TestTensorGlyph(Testing.vtkTest):
    def testGlyphs(self):
        '''Test if the glyphs are created nicely.'''
        reader = vtk.vtkDataSetReader()

        data_file = os.path.join(Testing.VTK_DATA_ROOT, "Data", "tensors.vtk")

        reader.SetFileName(data_file)

        g1 = SimpleGlyph(reader)
        g1.glyph.ColorGlyphsOff()
        g1.Update()

        g2 = SimpleGlyph(reader)
        g2.glyph.ExtractEigenvaluesOff()
        g2.Update()
        g2.SetPosition((2.0, 0.0, 0.0))

        g3 = SimpleGlyph(reader)
        g3.glyph.SetColorModeToEigenvalues()
        g3.glyph.ThreeGlyphsOn()
        g3.Update()
        g3.SetPosition((0.0, 2.0, 0.0))

        g4 = SimpleGlyph(reader)
        g4.glyph.SetColorModeToEigenvalues()
        g4.glyph.ThreeGlyphsOn()
        g4.glyph.SymmetricOn()
        g4.Update()
        g4.SetPosition((2.0, 2.0, 0.0))

        ren = vtk.vtkRenderer()
        for i in (g1, g2, g3, g4):
            for j in i.GetActors():
                ren.AddActor(j)

        ren.ResetCamera();

        cam = ren.GetActiveCamera()
        cam.Azimuth(-20)
        cam.Elevation(20)
        cam.Zoom(1.5)

        ren.SetBackground(0.5, 0.5, 0.5)

        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        renWin.Render()

        img_file = "TestTensorGlyph.png"
        Testing.compareImage(renWin, Testing.getAbsImagePath(img_file))
        Testing.interact()

    def testParse(self):
        "Test if vtkTensorGlyph is parseable"
        tg = vtk.vtkTensorGlyph()
        self._testParse(tg)

    def testGetSet(self):
        "Testing Get/Set methods of vtkTensorGlyph"
        tg = vtk.vtkTensorGlyph()
        self._testGetSet(tg)

    def testParse(self):
        "Testing Boolean methods of vtkTensorGlyph"
        tg = vtk.vtkTensorGlyph()
        self._testBoolean(tg)


if __name__ == "__main__":
    Testing.main([(TestTensorGlyph, 'test')])
