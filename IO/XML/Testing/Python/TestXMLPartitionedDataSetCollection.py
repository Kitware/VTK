from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkIOXML as ixml

from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir

import os

class TestXMLPartitionedDataSetCollection(Testing.vtkTest):

    def test(self):

        p = dm.vtkPartitionedDataSet()

        s = ic.vtkRTAnalyticSource()
        s.SetWholeExtent(0, 10, 0, 10, 0, 5)
        s.Update()

        p1 = dm.vtkImageData()
        p1.ShallowCopy(s.GetOutput())

        s.SetWholeExtent(0, 10, 0, 10, 5, 10)
        s.Update()

        p2 = dm.vtkImageData()
        p2.ShallowCopy(s.GetOutput())

        p.SetPartition(0, p1)
        p.SetPartition(1, p2)

        p2 = dm.vtkPartitionedDataSet()
        p2.ShallowCopy(p)

        c = dm.vtkPartitionedDataSetCollection()
        c.SetPartitionedDataSet(0, p)
        c.SetPartitionedDataSet(1, p2)

        tmpdir = vtkGetTempDir()
        fname = tmpdir+"/testxmlpartdscol.vtpc"
        w = ixml.vtkXMLPartitionedDataSetCollectionWriter()
        w.SetInputData(c)
        w.SetFileName(fname)
        w.Write()

        r = ixml.vtkXMLPartitionedDataSetCollectionReader()
        r.SetFileName(fname)
        r.Update()
        o = r.GetOutputDataObject(0)

        self.assertTrue(o.IsA("vtkPartitionedDataSetCollection"))
        nd = o.GetNumberOfPartitionedDataSets()
        self.assertEqual(nd, 2)

        for i in range(nd):
            p = o.GetPartitionedDataSet(i)
            p2 = c.GetPartitionedDataSet(i)
            self.assertTrue(p.IsA("vtkPartitionedDataSet"))
            self.assertEqual(p.GetNumberOfPartitions(), 2)
            self.assertEqual(p.GetPartition(0).GetNumberOfCells(), p.GetPartition(0).GetNumberOfCells())
        os.remove(fname)

if __name__ == "__main__":
    Testing.main([(TestXMLPartitionedDataSetCollection, 'test')])
