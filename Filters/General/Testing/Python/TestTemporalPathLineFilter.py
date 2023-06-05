import vtk
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

ts = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)
np = 3

class PointSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0,
                nOutputPorts=1, outputType='vtkUnstructuredGrid')
        self.Scale = 1.0
        self.FirstIteration = True

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
        if info.Has(vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS()):
            assert(self.FirstIteration !=
                    (info.Get(vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS()) ==
                        vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS_CONTINUE))
        pts = vtk.vtkPoints()
        pts.SetNumberOfPoints(3)
        pts.SetPoint(0, t * self.Scale, t * self.Scale, t)
        pts.SetPoint(1, t / self.Scale, t, t * self.Scale)
        pts.SetPoint(2, t * self.Scale, t / self.Scale, t)
        output.SetPoints(pts)
        self.FirstIteration = False
        return 1

ps = PointSource()

pathLines = vtk.vtkTemporalPathLineFilter()
pathLines.SetInputConnection(ps.GetOutputPort())
pathLines.SetMaskPoints(1)
pathLines.SetMaxTrackLength(30)
pathLines.SetMaxStepDistance(100, 100, 100)

# We are tosting the filter's ability to reinitialize
# by setting different scales
for scale in [1.0, 2.0]:
    psRef = PointSource()
    psRef.Scale = scale
    pathLinesRef = vtk.vtkTemporalPathLineFilter()
    pathLinesRef.SetInputConnection(psRef.GetOutputPort())
    pathLinesRef.SetMaskPoints(1)
    pathLinesRef.SetMaxTrackLength(30)
    pathLinesRef.SetMaxStepDistance(100, 100, 100)
    pathLinesRef.Update()

    ps.Scale = scale
    ps.FirstIteration = True
    ps.SetNoPriorTemporalAccessInformationKey()

    idx = 0
    for t in ts:
        pathLines.UpdateTimeStep(t)
        assert(pathLines.GetOutput().GetFieldData().GetArray("time_steps").GetValue(idx) == idx)
        idx += 1

    points = pathLines.GetOutput().GetPoints()
    pointsRef = pathLinesRef.GetOutput().GetPoints()

    assert(points.GetNumberOfPoints() == pointsRef.GetNumberOfPoints())
    assert(points.GetNumberOfPoints() == np * len(ts))
    for pointId in range(points.GetNumberOfPoints()):
        p = points.GetPoint(pointId)
        pRef = pointsRef.GetPoint(pointId)
        assert(p[0] == pRef[0] and p[1] == pRef[1] and p[2] == pRef[2])
