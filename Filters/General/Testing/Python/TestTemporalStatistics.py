import vtk
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

ts = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)

class PointSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0,
                nOutputPorts=1, outputType='vtkUnstructuredGrid')
        self.Scale = 1.0

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS(),
                    ts, len(ts))
        info.Set(vtk.vtkStreamingDemandDrivenPipeline.TIME_RANGE(),
                [ts[0], ts[-1]], 2)
        return 1
    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtk.vtkUnstructuredGrid.GetData(info)
        # The time step requested
        t = info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())
        pts = vtk.vtkPoints()
        pts.SetNumberOfPoints(1)
        pts.SetPoint(0, 0, 0, 0)
        output.SetPoints(pts)
        a = vtk.vtkFloatArray()
        a.SetName("scalar")
        a.SetNumberOfTuples(1)
        a.SetValue(0, t)
        output.GetPointData().AddArray(a)
        return 1

ps = PointSource()

stats = vtk.vtkTemporalStatistics()
stats.SetInputConnection(ps.GetOutputPort())

psRef = PointSource()

statsRef = vtk.vtkTemporalStatistics()
statsRef.SetInputConnection(psRef.GetOutputPort())

idx = 0
for scale in [1.0, 2,0]:

    psRef.Scale = scale
    psRef.Modified()
    statsRef.Update()

    avg = statsRef.GetOutput().GetPointData().GetArray("scalar_average").GetValue(0)
    max = statsRef.GetOutput().GetPointData().GetArray("scalar_maximum").GetValue(0)
    min = statsRef.GetOutput().GetPointData().GetArray("scalar_minimum").GetValue(0)
    stddev = statsRef.GetOutput().GetPointData().GetArray("scalar_stddev").GetValue(0)

    ps.SetIncompleteTimeStepsInformationKey(
            vtk.vtkStreamingDemandDrivenPipeline.INCOMPLETE_TIME_STEPS_RESET)
    ps.Scale = scale

    idx = 0
    for t in ts:
        stats.UpdateTimeStep(t)
        assert(stats.GetOutput().GetFieldData().GetArray("time_steps").GetValue(idx) == idx)
        ps.SetIncompleteTimeStepsInformationKey(
                vtk.vtkStreamingDemandDrivenPipeline.INCOMPLETE_TIME_STEPS_CONTINUE)
        idx += 1

    assert(avg == stats.GetOutput().GetPointData().GetArray("scalar_average").GetValue(0))
    assert(max == stats.GetOutput().GetPointData().GetArray("scalar_maximum").GetValue(0))
    assert(min == stats.GetOutput().GetPointData().GetArray("scalar_minimum").GetValue(0))
    assert(stddev == stats.GetOutput().GetPointData().GetArray("scalar_stddev").GetValue(0))
