"""Test support for fspath protocol VTK-Python

In VTK 3.6 and later, the Python open() method accepts pathlike objects,
such as pathlib.Path(), which represent file system paths and which
return a string via their __fspath__ slot.  This test checks that the
VTK SetFileName() methods accept pathlike objects just like open() does.

Created on March 27, 2021 by David Gobbi
"""

import sys
import os
from vtkmodules.vtkCommonCore import vtkFileOutputWindow
from vtkmodules.vtkCommonSystem import vtkDirectory
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetTempDir

VTK_TEMP_DIR = vtkGetTempDir()

pathstring = os.path.join(VTK_TEMP_DIR, "log.txt")

if sys.hexversion >= 0x03060000:
    from pathlib import Path
    pathobj = Path(pathstring)
else:
    Path = str
    pathobj = Path(pathstring)

class TestFilePath(Testing.vtkTest):
    def testSetFilePath(self):
        """Pass a path to a SetFileName method
        """
        w = vtkFileOutputWindow()
        w.SetFileName(pathobj)
        self.assertEqual(w.GetFileName(), str(pathobj))

    def testtDirectory(self):
        """Check the hinted vtkDirectory methods
        """
        d = vtkDirectory()
        # This will raise an exception if Path isn't accepted
        d.Open(Path(VTK_TEMP_DIR))

if __name__ == "__main__":
    Testing.main([(TestFilePath, 'test')])
