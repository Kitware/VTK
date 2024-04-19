#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkCommand,
    vtkMath,
)
from vtkmodules.vtkCommonDataModel import vtkPiecewiseFunction
from vtkmodules.vtkFiltersGeneral import vtkBrownianPoints
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDiscretizableColorTransferFunction,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

def SetRandomSeed(caller, eventId):
    #print "Restart random number generator"
    raMath = vtkMath()
    raMath.RandomSeed(6)


def SphereActor(lut, interpolateBeforeMapping):
    ss = vtkSphereSource()
    if interpolateBeforeMapping:
        ss.SetCenter(-1, 0, 0)

    bp = vtkBrownianPoints()
    bp.SetInputConnection(ss.GetOutputPort())
    bp.AddObserver (vtkCommand.EndEvent, SetRandomSeed)

    pm = vtkPolyDataMapper()
    pm.SetInputConnection(bp.GetOutputPort())
    pm.SetScalarModeToUsePointFieldData()
    pm.SelectColorArray("BrownianVectors")
    pm.SetLookupTable(lut)
    pm.SetInterpolateScalarsBeforeMapping(interpolateBeforeMapping)

    a = vtkActor()
    a.SetMapper(pm)
    return a

def ColorTransferFunction():
    opacityTransfer = vtkPiecewiseFunction()
    opacityTransfer.AddPoint(0,0)
    opacityTransfer.AddPoint(0.6,0)
    opacityTransfer.AddPoint(1,1)

    lut = vtkDiscretizableColorTransferFunction()
    lut.SetColorSpaceToDiverging();
    lut.AddRGBPoint(0.0, 0.23, 0.299, 0.754)
    lut.AddRGBPoint(1.0, 0.706, 0.016, 0.150);
    lut.SetVectorModeToMagnitude()
    lut.SetRange (0, 1)
    lut.SetScalarOpacityFunction(opacityTransfer)
    lut.EnableOpacityMappingOn()
    return lut


renWin = vtkRenderWindow()
renWin.SetSize(300, 300)
# enable depth peeling
renWin.AlphaBitPlanesOn()
renWin.SetMultiSamples(0)

ren = vtkRenderer()
ren.SetBackground(0, 0, 0)
# enable depth peeling
ren.UseDepthPeelingOn()
ren.SetMaximumNumberOfPeels(4)
renWin.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
SetRandomSeed(0, 0)

lut = ColorTransferFunction()
ren.AddActor(SphereActor (lut, 0))
ren.AddActor(SphereActor (lut, 1))

renWin.Render()
#iren.Start()
