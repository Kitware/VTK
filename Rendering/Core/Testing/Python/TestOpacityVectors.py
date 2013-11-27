#!/usr/bin/env python
import vtk

def SetRandomSeed(caller, eventId):
    #print "Restart random number generator"
    raMath = vtk.vtkMath()
    raMath.RandomSeed(6)


def SphereActor(lut, interpolateBeforeMapping):
    ss = vtk.vtkSphereSource()
    if interpolateBeforeMapping:
        ss.SetCenter(-1, 0, 0)

    bp = vtk.vtkBrownianPoints()
    bp.SetInputConnection(ss.GetOutputPort())
    bp.AddObserver (vtk.vtkCommand.EndEvent, SetRandomSeed)

    pm = vtk.vtkPolyDataMapper()
    pm.SetInputConnection(bp.GetOutputPort())
    pm.SetScalarModeToUsePointFieldData()
    pm.SelectColorArray("BrownianVectors")
    pm.SetLookupTable(lut)
    pm.SetInterpolateScalarsBeforeMapping(interpolateBeforeMapping)

    a = vtk.vtkActor()
    a.SetMapper(pm)
    return a

def ColorTransferFunction():
    opacityTransfer = vtk.vtkPiecewiseFunction()
    opacityTransfer.AddPoint(0,0)
    opacityTransfer.AddPoint(0.6,0)
    opacityTransfer.AddPoint(1,1)

    lut = vtk.vtkDiscretizableColorTransferFunction()
    lut.SetColorSpaceToDiverging();
    lut.AddRGBPoint(0.0, 0.23, 0.299, 0.754)
    lut.AddRGBPoint(1.0, 0.706, 0.016, 0.150);
    lut.SetVectorModeToMagnitude()
    lut.SetRange (0, 1)
    lut.SetScalarOpacityFunction(opacityTransfer)
    lut.EnableOpacityMappingOn()
    return lut


renWin = vtk.vtkRenderWindow()
renWin.SetSize(300, 300)
# enable depth peeling
renWin.AlphaBitPlanesOn()
renWin.SetMultiSamples(0)

ren = vtk.vtkRenderer()
ren.SetBackground(0, 0, 0)
# enable depth peeling
ren.UseDepthPeelingOn()
ren.SetMaximumNumberOfPeels(4)
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
SetRandomSeed(0, 0)

lut = ColorTransferFunction()
ren.AddActor(SphereActor (lut, 0))
ren.AddActor(SphereActor (lut, 1))

renWin.Render()
#iren.Start()
