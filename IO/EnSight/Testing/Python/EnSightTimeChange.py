#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Make sure all algorithms use the composite data pipeline
cdp = vtk.vtkCompositeDataPipeline()

reader = vtk.vtkGenericEnSightReader()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/particles/particles.case")
reader.ReadAllVariablesOn()
reader.UpdateTimeStep(0.000375)
reader.UpdateTimeStep(9.9803)
reader.UpdateTimeStep(0.000375)
reader.UpdateTimeStep(9.9803)

# Leaks without this line
reader.SetDefaultExecutivePrototype(None)
