import vtk

from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

class MovingSphereSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
            nInputPorts=0,
            nOutputPorts=1, outputType='vtkPolyData')

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        t = range(0, 10)
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtk.vtkPolyData.GetData(outInfo)

        t = info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        sphere = vtk.vtkSphereSource()
        sphere.SetCenter(0, t * 2, 0)
        sphere.Update()

        output.ShallowCopy(sphere.GetOutput())
        return 1

class MovingPDC(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
            nInputPorts=0,
            nOutputPorts=1, outputType='vtkPartitionedDataSetCollection')

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        t = range(0, 10)
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtk.vtkDataObject.GetData(outInfo)

        t = info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        source = vtk.vtkPartitionedDataSetCollectionSource()
        source.SetNumberOfShapes(int(1 + t % 3))

        transform = vtk.vtkTransform()
        transform.Identity()
        transform.Translate(2, t*2, 0)

        xform = vtk.vtkTransformPolyDataFilter()
        xform.SetTransform(transform)
        xform.SetInputConnection(source.GetOutputPort())
        xform.Update()

        output.ShallowCopy(xform.GetOutputDataObject(0))
        return 1

source1 = MovingSphereSource()
group1 = vtk.vtkGroupTimeStepsFilter()
group1.SetInputConnection(source1.GetOutputPort())
mapper1 = vtk.vtkCompositePolyDataMapper2()
mapper1.SetInputConnection(group1.GetOutputPort())
actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)

source2 = MovingPDC()
group2 = vtk.vtkGroupTimeStepsFilter()
group2.SetInputConnection(source2.GetOutputPort())
mapper2 = vtk.vtkCompositePolyDataMapper2()
mapper2.SetInputConnection(group2.GetOutputPort())
actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(actor1)
ren1.AddActor(actor2)

ren1.ResetCamera()
renWin.SetSize(300, 300)

renWin.Render()

# render the image
#
iren.Initialize()
# iren.Start()
