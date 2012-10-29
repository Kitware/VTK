#!/usr/bin/env python

"""
This file tests vtk.util.vtkImageExportToArray and
vtk.util.vtkImageImportFromArray.  It tests the code by first
exporting a PNG image to a Numeric Array and then converts the array
to an image and compares that image to the original image.  It does
this for all PNG images in a particular directory.

The test naturally requires Numeric Python to be installed:
  http://numpy.sf.net

Run this test like so:
vtkpython TestNumericArrayImageData.py -B $VTK_DATA_ROOT/Baseline/Imaging

"""

# This test requires Numeric.
import sys
try:
    import numpy.core.numeric as numeric
except ImportError:
    print "This test requires Numeric Python: http://numpy.sf.net"
    sys.exit(1)

import os
import glob
import vtk
from vtk.test import Testing
from vtk.util.vtkImageExportToArray import vtkImageExportToArray
from vtk.util.vtkImageImportFromArray import vtkImageImportFromArray


class TestNumericArrayImageData(Testing.vtkTest):
    def testImportExport(self):
        "Testing if images can be imported to and from numeric arrays."
        imp = vtkImageImportFromArray()
        exp = vtkImageExportToArray()
        idiff = vtk.vtkImageDifference()

        img_dir = Testing.getAbsImagePath("")
        for i in glob.glob(os.path.join(img_dir, "*.png")):
            # Putting the reader outside the loop causes bad problems.
            reader = vtk.vtkPNGReader()
            reader.SetFileName(i)
            reader.Update()

            # convert the image to a Numeric Array and convert it back
            # to an image data.
            exp.SetInputConnection(reader.GetOutputPort())
            imp.SetArray(exp.GetArray())

            # ensure there is no difference between orig image and the
            # one we converted and un-converted.
            idiff.SetInputConnection(imp.GetOutputPort())
            idiff.SetImage(reader.GetOutput())
            idiff.Update()
            err = idiff.GetThresholdedError()

            msg = "Test failed on image %s, with threshold "\
                  "error: %d"%(i, err)
            self.assertEqual(err, 0.0, msg)


if __name__ == "__main__":
    Testing.main([(TestNumericArrayImageData, 'test')])
