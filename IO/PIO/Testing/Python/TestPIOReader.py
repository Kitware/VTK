#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

controller = vtk.vtkMultiProcessController.GetGlobalController()
rank = controller.GetLocalProcessId()

pioreader = vtk.vtkPIOReader()
pioreader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/PIO/simple.pio")
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
geometryFilter = vtk.vtkHyperTreeGridGeometry()
geometryFilter.SetInputData(piece)
geometryFilter.Update()

# ---------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------

ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

syncWindows = vtk.vtkSynchronizedRenderWindows()
syncWindows.SetRenderWindow(renWin)
syncWindows.SetParallelController(controller)
syncWindows.SetIdentifier(1)

syncRenderers = vtk.vtkCompositedSynchronizedRenderers()
syncRenderers.SetRenderer(ren);
syncRenderers.SetParallelController(controller);

lut = vtk.vtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.SetSaturationRange(1.0, 0.25);
lut.SetTableRange(48.5, 50)
lut.Build()

mapper = vtk.vtkDataSetMapper()
mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('cell_energy')

mapper.SetInputConnection(geometryFilter.GetOutputPort())

mapper.UseLookupTableScalarRangeOn()
mapper.SetScalarRange(48.5, 50)

actor = vtk.vtkActor()
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
