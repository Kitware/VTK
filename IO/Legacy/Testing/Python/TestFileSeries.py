 #!/usr/bin/env python

import os
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkIOLegacy import vtkPolyDataReader
from vtkmodules.util.misc import vtkGetDataRoot

r = vtkPolyDataReader()
r.AddFileName(vtkGetDataRoot() + "/Data/track1.binary.vtk")
r.AddFileName(vtkGetDataRoot() + "/Data/track2.binary.vtk")
r.AddFileName(vtkGetDataRoot() + "/Data/track3.binary.vtk")
r.UpdateInformation()

assert(r.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS()) == 3)

r.UpdateTimeStep(1)
rng = r.GetOutput().GetPointData().GetScalars().GetRange(0)
assert(abs(rng[0] - 0.4) < 0.0001)
r.UpdateTimeStep(2)
rng = r.GetOutput().GetPointData().GetScalars().GetRange(0)
assert(abs(rng[1] + 5.8 ) < 0.0001)
