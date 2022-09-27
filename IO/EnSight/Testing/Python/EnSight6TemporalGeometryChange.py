#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtk.vtkGenericEnSightReader()
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/temporalCache.case")
tCache = vtk.vtkTemporalDataSetCache()
tCache.SetInputConnection(reader.GetOutputPort())
tCache.SetCacheSize(3)

# populate cache
tCache.UpdateTimeStep(0.0000)
tCache.UpdateTimeStep(0.010002)
tCache.UpdateTimeStep(0.020032)


# read through cache again
tCache.UpdateTimeStep(0.0000)
tCache.UpdateTimeStep(0.010002) # used to crash here

sphere = vtk.vtkSphereSource()
sphere.SetRadius(0.001)
mapper = vtk.vtkGlyph3DMapper()
mapper.SetInputConnection(tCache.GetOutputPort())
mapper.SetSourceConnection(sphere.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)
ren1.ResetCamera()
iren.Initialize()
renWin.Render()

# Leaks without this line
reader.SetDefaultExecutivePrototype(None)
