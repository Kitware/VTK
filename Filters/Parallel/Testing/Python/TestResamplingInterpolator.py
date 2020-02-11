from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from vtkmodules.vtkIOExodus import vtkExodusIIReader
from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
from vtkmodules.vtkCommonExecutionModel import vtkAlgorithm, vtkCompositeDataPipeline
from vtkmodules.vtkFiltersParallel import vtkAdaptiveTemporalInterpolator
from vtkmodules.vtkFiltersGeometry import vtkCompositeDataGeometryFilter
from vtkmodules.vtkRenderingCore import vtkCompositePolyDataMapper, vtkActor, vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor, vtkWindowToImageFilter
from vtkmodules.vtkRenderingOpenGL2 import vtkCompositePolyDataMapper2


class SimpleTimeReader(VTKPythonAlgorithmBase):
    """A reader that exposes two exodus files as a time series"""
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0, nOutputPorts=1, outputType='vtkMultiBlockDataSet')

        r1 = vtkExodusIIReader()
        r1.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/simpleamrgrid.e-s000")
        r1.SetElementBlockArrayStatus("Unnamed block ID: 12", 1)
        r1.SetElementResultArrayStatus("cell_dist", 1)
        r1.SetElementResultArrayStatus("cell_poly", 1)
        r1.SetPointResultArrayStatus("point_dist", 1)
        r1.SetPointResultArrayStatus("point_poly", 1)
        r1.Update()

        r2 = vtkExodusIIReader()
        r2.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/simpleamrgrid.e-s001")
        r2.SetElementBlockArrayStatus("Unnamed block ID: 12", 1)
        r2.SetElementResultArrayStatus("cell_dist", 1)
        r2.SetElementResultArrayStatus("cell_poly", 1)
        r2.SetPointResultArrayStatus("point_dist", 1)
        r2.SetPointResultArrayStatus("point_poly", 1)
        r2.Update()

        self._dataobjects = [r1.GetOutputDataObject(0), r2.GetOutputDataObject(0)]

    def _get_timesteps(self):
        return [0, 1]

    def _get_update_time(self, outInfo):
        executive = self.GetExecutive()
        if outInfo.Has(executive.UPDATE_TIME_STEP()):
            utime = outInfo.Get(executive.UPDATE_TIME_STEP())
            if utime < 0.5:
                return 0
            else:
                return 1
        else:
            return 0

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        executive = self.GetExecutive()
        outInfo = outInfoVec.GetInformationObject(0)
        outInfo.Remove(executive.TIME_STEPS())
        outInfo.Remove(executive.TIME_RANGE())

        timesteps = self._get_timesteps()
        if timesteps is not None:
            for t in timesteps:
                outInfo.Append(executive.TIME_STEPS(), t)
            outInfo.Append(executive.TIME_RANGE(), timesteps[0])
            outInfo.Append(executive.TIME_RANGE(), timesteps[-1])
        return 1

    def RequestData(self, request, inInfoVec, outInfoVec):
        data_time = self._get_update_time(outInfoVec.GetInformationObject(0))

        output = vtkMultiBlockDataSet.GetData(outInfoVec, 0)

        output.DeepCopy(self._dataobjects[data_time])

        output.GetInformation().Set(output.DATA_TIME_STEP(), data_time)

        return 1


prototype = vtkCompositeDataPipeline()
vtkAlgorithm.SetDefaultExecutivePrototype(prototype)
prototype.FastDelete()

reader = SimpleTimeReader()

interp = vtkAdaptiveTemporalInterpolator()
interp.SetInputConnection(reader.GetOutputPort())

geom = vtkCompositeDataGeometryFilter()
geom.SetInputConnection(interp.GetOutputPort())

# map them
mapper = vtkCompositePolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray('point_poly')
mapper.SetScalarRange(1.0, 6.0)
mapper.InterpolateScalarsBeforeMappingOn()
mapper.SetScalarVisibility(1)

actor = vtkActor()
actor.SetMapper(mapper)

renderer = vtkRenderer()
renWin = vtkRenderWindow()
iren = vtkRenderWindowInteractor()

renderer.AddActor(actor)
renderer.SetBackground(0.0, 0.0, 0.0)

renWin.AddRenderer(renderer)
renWin.SetSize(300, 300)
iren.SetRenderWindow(renWin)

# ask for some specific data points
info = geom.GetOutputInformation(0)
geom.UpdateInformation()

time = 0.5

info.Set(prototype.UPDATE_TIME_STEP(), time)
mapper.Modified()
renderer.ResetCameraClippingRange()
renWin.Render()
