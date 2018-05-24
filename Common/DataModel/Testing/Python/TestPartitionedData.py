from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtk.test import Testing

class SimpleFilter(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self)
        self.InputType = "vtkDataSet"
        self.OutputType = "vtkDataObject"
        self.Counter = 0

    def FillInputPortInformation(self, port, info):
        """Sets the required input type to InputType."""
        info.Set(em.vtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), self.InputType)
        info.Append(em.vtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet")
        return 1

    def RequestDataObject(self, request, inInfo, outInfo):
        inp = dm.vtkDataObject.GetData(inInfo[0])
        opt = dm.vtkDataObject.GetData(outInfo)

        if opt and opt.IsA(inp.GetClassName()):
            return 1

        opt = inp.NewInstance()
        outInfo.GetInformationObject(0).Set(dm.vtkDataObject.DATA_OBJECT(), opt)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        global key
        inp = dm.vtkDataObject.GetData(inInfo[0])
        opt = dm.vtkDataObject.GetData(outInfo)
        a = cc.vtkTypeUInt8Array()
        a.SetName("counter")
        a.SetNumberOfTuples(1)
        a.SetValue(0, self.Counter)
        opt.GetFieldData().AddArray(a)
        self.Counter += 1
        return 1

class TestPartitionedData(Testing.vtkTest):

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

        s = SimpleFilter()
        s.SetInputDataObject(c)
        s.Update()
        for i in (0,1):
            self.assertEqual(s.GetOutputDataObject(0).GetPartitionedDataSet(i).GetFieldData().GetArray("counter").GetValue(0), i)

if __name__ == "__main__":
    Testing.main([(TestPartitionedData, 'test')])
