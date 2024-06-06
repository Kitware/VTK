import vtk
import math
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase

# timesteps
ts = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)

# threshold criteria
BETWEEN = 0
LOWER = 1
UPPER = 2

# Custom source, geometry is a single point with different scalar & vector point data
# arrays generated depending on requested timestep
class PointSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0,
                nOutputPorts=1, outputType='vtkUnstructuredGrid')
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
        # Check the NO_PRIOR_TEMPORAL_ACCESS() value (vtkTemporalAlgorithm-specific)
        timestep = info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())
        if info.Has(vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS()):
            assert(self.FirstIteration !=
                    (info.Get(vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS()) ==
                        vtk.vtkStreamingDemandDrivenPipeline.NO_PRIOR_TEMPORAL_ACCESS_CONTINUE))
        pts = vtk.vtkPoints()
        pts.SetNumberOfPoints(1)
        pts.SetPoint(0, 0, 0, 0)
        output.SetPoints(pts)

        scalars = vtk.vtkFloatArray()
        scalars.SetName("scalars")
        scalars.SetNumberOfTuples(1)
        scalars.SetValue(0, timestep)
        output.GetPointData().AddArray(scalars)

        inv_scalars = vtk.vtkFloatArray()
        inv_scalars.SetName("inv_scalars")
        inv_scalars.SetNumberOfTuples(1)
        inv_scalars.SetValue(0, -timestep)
        output.GetPointData().AddArray(inv_scalars)

        vectors = vtk.vtkDoubleArray()
        vectors.SetName("vectors")
        vectors.SetNumberOfComponents(3)
        vectors.SetNumberOfTuples(1)
        vectors.SetTuple3(0, timestep, -timestep, 2*timestep)
        output.GetPointData().AddArray(vectors)

        self.FirstIteration = False
        return 1

# Test vtkCriticalTime filter with single-component array
def test_ctt_filter_scalar(criticalTT):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "scalars")
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(5.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("scalars_critical_time").GetValue(0) == 5.0)

    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "inv_scalars")
    criticalTT.SetThresholdCriterion(LOWER)
    criticalTT.SetLowerThreshold(-4.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("inv_scalars_critical_time").GetValue(0) == 4.0)

    criticalTT.SetThresholdCriterion(BETWEEN)
    criticalTT.SetUpperThreshold(-1.0)
    criticalTT.SetLowerThreshold(-3.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("inv_scalars_critical_time").GetValue(0) == 1.0)

    criticalTT.SetUpperThreshold(-3.0)
    criticalTT.SetLowerThreshold(-1.0) # Reverted values, never reached
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("inv_scalars_critical_time").GetValue(0)))

# Test vtkCriticalTime filter with 3-component array
# With "Use Selected" component mode enabled
def test_ctt_filter_vector_comp(criticalTT):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "vectors")
    criticalTT.SetComponentModeToUseSelected()
    criticalTT.SetSelectedComponent(0) # X
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(5.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 5.0)

    criticalTT.SetSelectedComponent(1) # Y
    criticalTT.SetThresholdCriterion(LOWER)
    criticalTT.SetLowerThreshold(-4.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 4.0)

    criticalTT.SetSelectedComponent(2) # Z
    criticalTT.SetThresholdCriterion(BETWEEN)
    criticalTT.SetLowerThreshold(6)
    criticalTT.SetUpperThreshold(7)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 3.0)

# Test vtkCriticalTime filter with 3-component array
# With "Use Selected" component mode enabled, magnitude case
def test_ctt_filter_vector_mag(criticalTT):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "vectors")
    criticalTT.SetComponentModeToUseSelected()
    criticalTT.SetSelectedComponent(3) # Mag
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(math.sqrt(2*2 + 2*2 + 4*4)) # timestep = 2
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 2.0)

    criticalTT.SetThresholdCriterion(LOWER)
    criticalTT.SetLowerThreshold(-1.0) # Never reached
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0)))

    criticalTT.SetThresholdCriterion(BETWEEN)
    criticalTT.SetLowerThreshold(2.5)
    criticalTT.SetUpperThreshold(3.5) # Never comprised between these 2 values
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0)))

# Test vtkCriticalTime filter with 3-component array
# With "Use All" component mode enabled
def test_ctt_filter_vector_all(criticalTT):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "vectors")
    criticalTT.SetComponentModeToUseAll()
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(1.0) # Never reached by all at the same time
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0)))

    criticalTT.SetThresholdCriterion(LOWER)
    criticalTT.SetLowerThreshold(4.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 0.0)

    criticalTT.SetThresholdCriterion(BETWEEN)
    criticalTT.SetLowerThreshold(1.0)
    criticalTT.SetUpperThreshold(5.0) # Never reached by all at the same time
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0)))

# Test vtkCriticalTime filter with 3-component array
# With "Use Any" component mode enabled
def test_ctt_filter_vector_any(criticalTT):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "vectors")
    criticalTT.SetComponentModeToUseAny()
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(16.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 8.0)

    criticalTT.SetThresholdCriterion(LOWER)
    criticalTT.SetLowerThreshold(-16.0) # Never reached
    criticalTT.Update()
    assert(math.isnan(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0)))

    criticalTT.SetThresholdCriterion(BETWEEN)
    criticalTT.SetUpperThreshold(-5.0)
    criticalTT.SetLowerThreshold(-7.0)
    criticalTT.Update()
    assert(criticalTT.GetOutput().GetPointData().GetArray("vectors_critical_time").GetValue(0) == 5)

# Test vtkCriticalTime filter with 3-component array
def test_ctt_filter_vector(criticalTT):
    test_ctt_filter_vector_comp(criticalTT)
    test_ctt_filter_vector_mag(criticalTT)
    test_ctt_filter_vector_all(criticalTT)
    test_ctt_filter_vector_any(criticalTT)

# Check "time_steps" array has been generated with NoPriorTimeStepAccess set
# (e.g. in in-situ case, vtkTemporalAlgorithm-specific)
# Also check we obtain the same result than "standard" filter when all
# timesteps has been processed in this mode
def test_in_situ(criticalTT, criticalTTIS):
    criticalTT.SetInputArrayToProcess(0, 0, 0, 0, "scalars")
    criticalTT.SetThresholdCriterion(UPPER)
    criticalTT.SetUpperThreshold(5.0)
    criticalTT.Update()

    criticalTTIS.SetInputArrayToProcess(0, 0, 0, 0, "scalars")
    criticalTTIS.SetThresholdCriterion(UPPER)
    criticalTTIS.SetUpperThreshold(5.0)

    idx = 0
    for timestep in ts:
        criticalTTIS.UpdateTimeStep(timestep)
        assert(criticalTTIS.GetOutput().GetFieldData().GetArray("time_steps").GetValue(idx) == idx)
        idx += 1

    assert(criticalTT.GetOutput().GetPointData().GetArray("scalars_critical_time").GetNumberOfValues()
        == criticalTTIS.GetOutput().GetPointData().GetArray("scalars_critical_time").GetNumberOfValues()
        == 1)

    assert(criticalTT.GetOutput().GetPointData().GetArray("scalars_critical_time").GetValue(0)
        == criticalTTIS.GetOutput().GetPointData().GetArray("scalars_critical_time").GetValue(0)
        == 5.0)

# Test vtkCriticalTime filter
pointSource = PointSource()

criticalTT = vtk.vtkCriticalTime()
criticalTT.SetInputConnection(pointSource.GetOutputPort())

test_ctt_filter_scalar(criticalTT)
test_ctt_filter_vector(criticalTT)

# Test vtkCriticalTime filter, in-situ case
pointSourceIS = PointSource()
pointSourceIS.SetNoPriorTemporalAccessInformationKey()

criticalTTIS = vtk.vtkCriticalTime()
criticalTTIS.SetInputConnection(pointSourceIS.GetOutputPort())

test_in_situ(criticalTT, criticalTTIS)
