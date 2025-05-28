#!/usr/bin/env python
#
# This test exercises usage of rshift operator to build pipelines with vtkAlgorithm and vtkDataObject.

import vtkmodules.test.Testing as vtkTesting

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    vtkTesting.skip()

class TestDataModel(vtkTesting.vtkTest):
    def testAttributes(self):
        from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSet
        from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
        from vtkmodules.vtkFiltersCore import vtkPointDataToCellData
        wlt = (vtkRTAnalyticSource() >> vtkPointDataToCellData(pass_point_data=True))()
        self.assertTrue('RTData' in wlt.point_data)
        self.assertTrue('RTData' in wlt.cell_data)
        rtdata = wlt.point_data['RTData']
        self.assertTrue(issubclass(type(rtdata), numpy.ndarray))
        self.assertTrue(rtdata.shape == (wlt.number_of_points,))
        rtdata = wlt.cell_data['RTData']
        self.assertTrue(issubclass(type(rtdata), numpy.ndarray))
        self.assertTrue(rtdata.shape == (wlt.number_of_cells,))
        wlt2 = wlt.NewInstance()
        wlt2.DeepCopy(wlt)
        pds = vtkPartitionedDataSet()
        pds.append(wlt)
        pds.append(wlt2)
        self.assertTrue('RTData' in pds.point_data)

        # test iterator + len
        wlt.point_data["RTData2"] = rtdata * 2
        for name in wlt.point_data:
            self.assertTrue(name in ('RTData','RTData2'))

        self.assertTrue(len(wlt.point_data) == 2)

        for name in wlt.cell_data:
            self.assertTrue(name in ('RTData',))

        self.assertTrue(len(wlt.cell_data) == 1)

if __name__ == "__main__":
    vtkTesting.main([(TestDataModel, "test")])
