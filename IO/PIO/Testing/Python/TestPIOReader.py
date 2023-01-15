#!/usr/bin/env python

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersHyperTree import vtkHyperTreeGridGeometry
from vtkmodules.vtkIOPIO import vtkPIOReader
from vtkmodules.vtkParallelCore import vtkMultiProcessController
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingParallel import (
    vtkCompositedSynchronizedRenderers,
    vtkSynchronizedRenderWindows,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

controller = vtkMultiProcessController.GetGlobalController()
rank = controller.GetLocalProcessId()

pioreader = vtkPIOReader()
pioreader.SetFileName(VTK_DATA_ROOT + "/Data/PIO/simple.pio")
pioreader.UpdateInformation()

# confirm default arrays are enabled
default_arrays = ["tev", "pres", "rade", "cell_energy", "kemax",
        "vel", "eng"]
selection = pioreader.GetCellDataArraySelection()
for name in default_arrays:
    if not selection.ArrayExists(name) or selection.ArrayIsEnabled(name):
        # all's well
        pass
    else:
        raise RuntimeError("'%s' should have been enabled by default." % name)

pioreader.SetCurrentTimeStep(1)
pioreader.Update()

grid = pioreader.GetOutput()
block = grid.GetBlock(0)
piece = block.GetPieceAsDataObject(rank)
geometryFilter = vtkHyperTreeGridGeometry()
geometryFilter.SetInputData(piece)
geometryFilter.Update()

# ---------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

syncWindows = vtkSynchronizedRenderWindows()
syncWindows.SetRenderWindow(renWin)
syncWindows.SetParallelController(controller)
syncWindows.SetIdentifier(1)

syncRenderers = vtkCompositedSynchronizedRenderers()
syncRenderers.SetRenderer(ren);
syncRenderers.SetParallelController(controller);

lut = vtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.SetSaturationRange(1.0, 0.25);
lut.SetTableRange(48.5, 50)
lut.Build()

mapper = vtkDataSetMapper()
mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('cell_energy')

mapper.SetInputConnection(geometryFilter.GetOutputPort())

mapper.UseLookupTableScalarRangeOn()
mapper.SetScalarRange(48.5, 50)

actor = vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)
renWin.SetSize(300,300)
ren.SetBackground(0.5,0.5,0.5)

ren.GetActiveCamera().SetPosition(0.0108652, -0.00586516, 0.0143301)
ren.GetActiveCamera().SetViewUp(0.707107, 0.707107, 0)
ren.GetActiveCamera().SetParallelScale(0.00433013)
ren.GetActiveCamera().SetFocalPoint(0.0025, 0.0025, 0.0025)

renWin.Render()

# prevent the tk window from showing up then start the event loop
pioreader.SetDefaultExecutivePrototype(None)
