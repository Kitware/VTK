#!/usr/bin/env python

# Test the HDF5 functionality of the PIO Reader

from vtkmodules.vtkCommonCore import vtkLookupTable
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
pioreader.SetFileName(VTK_DATA_ROOT + "/Data/PIO/simple_h5.pio")
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
lut.SetHueRange(0.77, 0)
lut.SetSaturationRange(1.0, 0.65);
lut.SetTableRange(48.5, 62.29)
lut.Build()

mapper = vtkDataSetMapper()
mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('cell_energy')

mapper.SetInputData(piece)

mapper.UseLookupTableScalarRangeOn()
mapper.SetScalarRange(48.5, 62.29)

actor = vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)
renWin.SetSize(300,300)
ren.SetBackground(0.5,0.5,0.5)

ren.GetActiveCamera().SetPosition(0.0189, -0.0109, 0.0160)
ren.GetActiveCamera().SetViewUp(0.444, 0.8211, 0.3576)
ren.GetActiveCamera().SetParallelScale(0.0061)
ren.GetActiveCamera().SetFocalPoint(0.0049, 0.002, 0.0024)

renWin.Render()
#iren.GetRenderWindow().Render()

# prevent the tk window from showing up then start the event loop
pioreader.SetDefaultExecutivePrototype(None)
