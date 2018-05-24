from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkIOXML as ixml

from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir

import os

class TestXMLPartitionedDataSet(Testing.vtkTest):

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

        tmpdir = vtkGetTempDir()
        fname = tmpdir+"/testxmlpartds.vtpd"
        w = ixml.vtkXMLPartitionedDataSetWriter()
        w.SetInputData(p)
        w.SetFileName(fname)
        w.Write()

        r = ixml.vtkXMLPartitionedDataSetReader()
        r.SetFileName(fname)
        r.Update()
        o = r.GetOutputDataObject(0)

        print(o.IsA("vtkPartitionedDataSet"))
        np = o.GetNumberOfPartitions()
        self.assertEqual(np, 2)

        for i in range(np):
            d = o.GetPartition(i)
            d2 = p.GetPartition(i)
            self.assertTrue(d.IsA("vtkImageData"))
            self.assertEqual(d.GetNumberOfCells(), d2.GetNumberOfCells())
        os.remove(fname)

if __name__ == "__main__":
    Testing.main([(TestXMLPartitionedDataSet, 'test')])
