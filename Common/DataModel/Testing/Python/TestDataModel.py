#!/usr/bin/env python
#
# This test exercises usage of rshift operator to build pipelines with vtkAlgorithm and vtkDataObject.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

try:
    import numpy
    from vtkmodules.vtkIOIOSS import vtkIOSSReader
except ImportError:
    print("This test requires numpy and vtkIOSSReader!")
    vtkTesting.skip()

class TestDataModel(vtkTesting.vtkTest):
    def testCompositeAttributes(self):
        reader = vtkIOSSReader()
        reader.SetFileName(VTK_DATA_ROOT + "/Data/disk_out_ref.ex2")
        ds = reader()
        self.assertTrue(len(ds.point_data) == 8)
        self.assertTrue(len(ds.cell_data) == 2)
        self.assertTrue(len(ds.field_data) == 0) # not listing string arrays? (should be 2)

        for name in ds.point_data:
            self.assertTrue(name in {"CH4", "AsH3", "GaMe3", "H2", "ids", "Pres", "Temp", "V"})
        for name in ds.cell_data:
            self.assertTrue(name in {"object_id", "ids"})
        # for name in ds.field_data:
        #     self.assertTrue(name in {"Information Records", "QA Records"})

if __name__ == "__main__":
    vtkTesting.main([(TestDataModel, "test")])
