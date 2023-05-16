from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkPolyData,
)
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import (
    vtkGroupTimeStepsFilter,
    vtkTransformPolyDataFilter,
)
from vtkmodules.vtkFiltersSources import (
    vtkPartitionedDataSetCollectionSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

class MovingSphereSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
            nInputPorts=0,
            nOutputPorts=1, outputType='vtkPolyData')

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        t = range(0, 10)
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtkPolyData.GetData(outInfo)

        t = info.Get(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        sphere = vtkSphereSource()
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
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtkDataObject.GetData(outInfo)

        t = info.Get(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        source = vtkPartitionedDataSetCollectionSource()
        source.SetNumberOfShapes(int(1 + t % 3))

        transform = vtkTransform()
        transform.Identity()
        transform.Translate(2, t*2, 0)

        xform = vtkTransformPolyDataFilter()
        xform.SetTransform(transform)
        xform.SetInputConnection(source.GetOutputPort())
        xform.Update()

        output.ShallowCopy(xform.GetOutputDataObject(0))
        return 1

source1 = MovingSphereSource()
group1 = vtkGroupTimeStepsFilter()
group1.SetInputConnection(source1.GetOutputPort())
mapper1 = vtkCompositePolyDataMapper()
mapper1.SetInputConnection(group1.GetOutputPort())
actor1 = vtkActor()
actor1.SetMapper(mapper1)

source2 = MovingPDC()
group2 = vtkGroupTimeStepsFilter()
group2.SetInputConnection(source2.GetOutputPort())
mapper2 = vtkCompositePolyDataMapper()
mapper2.SetInputConnection(group2.GetOutputPort())
actor2 = vtkActor()
actor2.SetMapper(mapper2)

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
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
