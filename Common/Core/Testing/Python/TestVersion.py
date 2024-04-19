"""Test getting __version__ for VTK package
"""

import vtkmodules
from vtkmodules.vtkCommonCore import vtkVersion
from vtkmodules.test import Testing

class TestVersion(Testing.vtkTest):
    def testVersionAttribute(self):
        """Test the __version__ attribute
        """
        x,y,z = vtkmodules.__version__.split(".")
        self.assertEqual(x, str(vtkVersion.GetVTKMajorVersion()))
        self.assertEqual(y, str(vtkVersion.GetVTKMinorVersion()))
        self.assertEqual(z, str(vtkVersion.GetVTKBuildVersion()))

if __name__ == "__main__":
    Testing.main([(TestVersion, 'test')])
