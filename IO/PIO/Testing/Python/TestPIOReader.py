#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

pioreader = vtk.vtkPIOReader()
pioreader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/PIO/simple.pio")
pioreader.SetCurrentTimeStep(1)
pioreader.Update()

grid = pioreader.GetOutput()
block = grid.GetBlock(0)
geometryFilter = vtk.vtkHyperTreeGridGeometry()
geometryFilter.SetInputData(block)
geometryFilter.Update()

# ---------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------

ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

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
ren.ResetCamera()
ren.GetActiveCamera().Roll(45);
ren.GetActiveCamera().Azimuth(45);
renWin.Render()

# prevent the tk window from showing up then start the event loop
pioreader.SetDefaultExecutivePrototype(None)
