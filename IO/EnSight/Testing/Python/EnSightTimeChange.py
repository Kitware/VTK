#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkCompositeDataPipeline
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Make sure all algorithms use the composite data pipeline
cdp = vtkCompositeDataPipeline()

reader = vtkGenericEnSightReader()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/particles/particles.case")
reader.ReadAllVariablesOn()
reader.UpdateTimeStep(0.000375)
reader.UpdateTimeStep(9.9803)
reader.UpdateTimeStep(0.000375)
reader.UpdateTimeStep(9.9803)

# Leaks without this line
reader.SetDefaultExecutivePrototype(None)
