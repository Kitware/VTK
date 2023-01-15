#!/usr/bin/env python
from vtkmodules.vtkFiltersHybrid import vtkTemporalDataSetCache
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkGlyph3DMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtkGenericEnSightReader()
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/temporalCache.case")
tCache = vtkTemporalDataSetCache()
tCache.SetInputConnection(reader.GetOutputPort())
tCache.SetCacheSize(3)

# populate cache
tCache.UpdateTimeStep(0.0000)
tCache.UpdateTimeStep(0.010002)
tCache.UpdateTimeStep(0.020032)


# read through cache again
tCache.UpdateTimeStep(0.0000)
tCache.UpdateTimeStep(0.010002) # used to crash here

sphere = vtkSphereSource()
sphere.SetRadius(0.001)
mapper = vtkGlyph3DMapper()
mapper.SetInputConnection(tCache.GetOutputPort())
mapper.SetSourceConnection(sphere.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)
ren1.ResetCamera()
iren.Initialize()
renWin.Render()

# Leaks without this line
reader.SetDefaultExecutivePrototype(None)
