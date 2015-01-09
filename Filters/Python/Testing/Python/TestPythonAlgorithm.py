import vtk
from vtk.util import vtkAlgorithm as vta
from vtk.test import Testing

class TestPythonAlgorithm(Testing.vtkTest):
    def testSource(self):
        class MyAlgorithm(vta.VTKAlgorithm):
            def __init__(self):
                vta.VTKAlgorithm.__init__(self, nInputPorts=0, outputType='vtkImageData')
                self.Wavelet = vtk.vtkRTAnalyticSource()

            def RequestInformation(self, vtkself, request, inInfo, outInfo):
                self.Wavelet.UpdateInformation()
                wOutInfo = self.Wavelet.GetOutputInformation(0)
                vtkSDDP = vtk.vtkStreamingDemandDrivenPipeline
                outInfo.GetInformationObject(0).Set(vtkSDDP.WHOLE_EXTENT(), wOutInfo.Get(vtkSDDP.WHOLE_EXTENT()), 6)
                return 1

            def RequestData(self, vtkself, request, inInfo, outInfo):
                self.Wavelet.Update()
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(self.Wavelet.GetOutput())
                return 1

        ex = vtk.vtkPythonAlgorithm()
        ex.SetPythonObject(MyAlgorithm())

        ex.Update()

        w = vtk.vtkRTAnalyticSource()
        w.Update()

        output = ex.GetOutputDataObject(0)
        self.assertEqual(output.GetPointData().GetScalars().GetRange(),\
            w.GetOutput().GetPointData().GetScalars().GetRange())
        vtkSDDP = vtk.vtkStreamingDemandDrivenPipeline
        self.assertEqual(ex.GetOutputInformation(0).Get(vtkSDDP.WHOLE_EXTENT()),\
            w.GetOutputInformation(0).Get(vtkSDDP.WHOLE_EXTENT()))

    def testSource2(self):
        class MyAlgorithm(vta.VTKPythonAlgorithmBase):
            def __init__(self):
                vta.VTKPythonAlgorithmBase.__init__(self, nInputPorts=0, outputType='vtkImageData')
                self.Wavelet = vtk.vtkRTAnalyticSource()

            def RequestInformation(self, request, inInfo, outInfo):
                self.Wavelet.UpdateInformation()
                wOutInfo = self.Wavelet.GetOutputInformation(0)
                vtkSDDP = vtk.vtkStreamingDemandDrivenPipeline
                outInfo.GetInformationObject(0).Set(
                    vtkSDDP.WHOLE_EXTENT(), wOutInfo.Get(vtkSDDP.WHOLE_EXTENT()), 6)
                return 1

            def RequestData(self, request, inInfo, outInfo):
                self.Wavelet.Update()
                out = vtk.vtkImageData.GetData(outInfo)
                out.ShallowCopy(self.Wavelet.GetOutput())
                return 1

        ex = MyAlgorithm()

        ex.Update()

        w = vtk.vtkRTAnalyticSource()
        w.Update()

        output = ex.GetOutputDataObject(0)
        self.assertEqual(output.GetPointData().GetScalars().GetRange(),\
            w.GetOutput().GetPointData().GetScalars().GetRange())
        vtkSDDP = vtk.vtkStreamingDemandDrivenPipeline
        self.assertEqual(ex.GetOutputInformation(0).Get(vtkSDDP.WHOLE_EXTENT()),\
            w.GetOutputInformation(0).Get(vtkSDDP.WHOLE_EXTENT()))

    def testFilter(self):
        class MyAlgorithm(vta.VTKAlgorithm):
            def RequestData(self, vtkself, request, inInfo, outInfo):
                inp = self.GetInputData(inInfo, 0, 0)
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(inp)
                return 1

        sphere = vtk.vtkSphereSource()

        ex = vtk.vtkPythonAlgorithm()
        ex.SetPythonObject(MyAlgorithm())

        ex.SetInputConnection(sphere.GetOutputPort())

        ex.Update()

        output = ex.GetOutputDataObject(0)
        ncells = output.GetNumberOfCells()
        self.assertNotEqual(ncells, 0)
        self.assertEqual(ncells, sphere.GetOutput().GetNumberOfCells())
        self.assertEqual(output.GetBounds(), sphere.GetOutput().GetBounds())

    def testFilter2(self):
        class MyAlgorithm(vta.VTKPythonAlgorithmBase):
            def __init__(self):
                vta.VTKPythonAlgorithmBase.__init__(self)
            def RequestData(self, request, inInfo, outInfo):
                inp = self.GetInputData(inInfo, 0, 0)
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(inp)
                return 1

        sphere = vtk.vtkSphereSource()

        ex = MyAlgorithm()
        ex.SetInputConnection(sphere.GetOutputPort())

        ex.Update()

        output = ex.GetOutputDataObject(0)
        ncells = output.GetNumberOfCells()
        self.assertNotEqual(ncells, 0)
        self.assertEqual(ncells, sphere.GetOutput().GetNumberOfCells())
        self.assertEqual(output.GetBounds(), sphere.GetOutput().GetBounds())

if __name__ == "__main__":
    Testing.main([(TestPythonAlgorithm, 'test')])
