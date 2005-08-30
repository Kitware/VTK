#!/usr/bin/env python
"""
Run this test like so:
$ vtkpython TestGL2PSExporter.py  -D $VTK_DATA_ROOT \
  -B $VTK_DATA_ROOT/Baseline/Rendering/

$ vtkpython TestGL2PSExporter.py --help
provides more details on other options.

Please note that this test requires that you have PIL (the Python
Imaging Library) module installed and also GL2PS support built into
VTK.

"""

import sys
import os
import os.path
import tempfile

import vtk
from vtk.test import Testing

# This requires that you have PIL installed.
try:
    import Image
except ImportError:
    print "Please install PIL (Python Imaging Library) to run this test."
    sys.exit(0)
    

class TestGL2PSExporter(Testing.vtkTest):
    # Create these as class attributes so that they are only created
    # once and not for every test.
    cs = vtk.vtkConeSource()
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(cs.GetOutputPort())

    act = vtk.vtkActor()
    act.SetMapper(mapper)
    act.GetProperty().SetColor(0.5, 0.5, 1.0)

    axes = vtk.vtkCubeAxesActor2D()
    axes.SetInputConnection(cs.GetOutputPort())
    axes.SetFontFactor(2.0)
    axes.SetCornerOffset(0.0)
    axes.GetProperty().SetColor(0,0,0)

    txt = vtk.vtkTextActor()
    txt.SetDisplayPosition(45, 150)
    txt.SetInput("This is test\nmultiline\ninput\ndata.")
    tprop = txt.GetTextProperty()
    tprop.SetFontSize(18)
    tprop.SetFontFamilyToArial()
    tprop.SetJustificationToCentered()
    tprop.BoldOn()
    tprop.ItalicOn()
    tprop.ShadowOn()
    tprop.SetColor(0, 0, 1)

    ren = vtk.vtkRenderer()
    ren.AddActor(act)
    ren.AddActor(txt)
    axes.SetCamera(ren.GetActiveCamera())
    ren.AddActor(axes)
    ren.SetBackground(0.8, 0.8, 0.8)

    renWin = vtk.vtkRenderWindow()
    renWin.AddRenderer(ren)

    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    cam = ren.GetActiveCamera()
    cam.Azimuth(30)

    iren.Initialize()
    iren.Render()

    def _cleanup(self, files):
        """Remove the list of given files."""
        for f in files:
            if os.path.isfile(f):
                os.remove(f)
        
    def testVectorEPS(self):
        """Test vector EPS output."""
        # Get a temporary file name.  Set the extension to empty since
        # the exporter appends a suitable extension.
        tmp_eps = tempfile.mktemp('')
        # Write an EPS file.
        exp = vtk.vtkGL2PSExporter()
        exp.SetRenderWindow(self.renWin)
        exp.SetFilePrefix(tmp_eps)
        # Turn off compression so PIL can read file.
        exp.CompressOff() 
        exp.SetSortToBSP()
        exp.DrawBackgroundOn()
        exp.Write()
        # Now read the EPS file using PIL.
        tmp_eps += '.eps'
        im = Image.open(tmp_eps)
        # Get a temporary name for the PNG file.
        tmp_png = tempfile.mktemp('.png')
        im.save(tmp_png)

        # Now read the saved image and compare it for the test.
        png_r = vtk.vtkPNGReader()
        png_r.SetFileName(tmp_png)
        png_r.Update()
        img = png_r.GetOutput()

        # Cleanup.  Do this first because if the test fails, an
        # exception is raised and the temporary files won't be
        # removed.
        self._cleanup([tmp_eps, tmp_png])
        
        img_file = "TestGL2PSExporter.png"
        Testing.compareImageWithSavedImage(img,
                                           Testing.getAbsImagePath(img_file))
        # Interact if necessary.
        Testing.interact()

    def testRasterEPS(self):
        """Test EPS output when Write3DPropsAsRasterImage is on."""
        # Get a temporary file name.  Set the extension to empty since
        # the exporter appends a suitable extension.
        tmp_eps = tempfile.mktemp('')
        # Write an EPS file.
        exp = vtk.vtkGL2PSExporter()
        exp.SetRenderWindow(self.renWin)
        exp.SetFilePrefix(tmp_eps)
        # Turn off compression so PIL can read file.
        exp.CompressOff() 
        exp.SetSortToOff()
        exp.DrawBackgroundOn()
        exp.Write3DPropsAsRasterImageOn()
        exp.Write()
        # Now read the EPS file using PIL.
        tmp_eps += '.eps'
        im = Image.open(tmp_eps)
        # Get a temporary name for the PNG file.
        tmp_png = tempfile.mktemp('.png')
        im.save(tmp_png)

        # Now read the saved image and compare it for the test.
        png_r = vtk.vtkPNGReader()
        png_r.SetFileName(tmp_png)
        png_r.Update()
        img = png_r.GetOutput()

        # Cleanup.  Do this first because if the test fails, an
        # exception is raised and the temporary files won't be
        # removed.
        self._cleanup([tmp_eps, tmp_png])
        
        img_file = "TestGL2PSExporter.png"
        Testing.compareImageWithSavedImage(img,
                                           Testing.getAbsImagePath(img_file))
        # Interact if necessary.
        Testing.interact()


if __name__ == "__main__":
    cases = [(TestGL2PSExporter, 'test')]
    # This should prevent debug leaks messages.
    del TestGL2PSExporter
    Testing.main(cases)
