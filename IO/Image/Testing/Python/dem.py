#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
Scale = 5
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.6,0)
lut.SetSaturationRange(1.0,0)
lut.SetValueRange(0.5,1.0)
demModel = vtk.vtkDEMReader()
demModel.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/SainteHelens.dem")
demModel.Update()
catch.catch(globals(),"""#demModel.Print()""")
lo = expr.expr(globals(), locals(),["Scale","*","lindex(demModel.GetElevationBounds(),0)"])
hi = expr.expr(globals(), locals(),["Scale","*","lindex(demModel.GetElevationBounds(),1)"])
demActor = vtk.vtkLODActor()
# create a pipeline for each lod mapper
lods = "4 8 16"
for lod in lods.split():
    locals()[get_variable_name("shrink", lod, "")] = vtk.vtkImageShrink3D()
    locals()[get_variable_name("shrink", lod, "")].SetShrinkFactors(expr.expr(globals(), locals(),["int","(","lod",")"]),expr.expr(globals(), locals(),["int","(","lod",")"]),1)
    locals()[get_variable_name("shrink", lod, "")].SetInputConnection(demModel.GetOutputPort())
    locals()[get_variable_name("shrink", lod, "")].AveragingOn()
    locals()[get_variable_name("geom", lod, "")] = vtk.vtkImageDataGeometryFilter()
    locals()[get_variable_name("geom", lod, "")].SetInputConnection(locals()[get_variable_name("shrink", lod, "")].GetOutputPort())
    locals()[get_variable_name("geom", lod, "")].ReleaseDataFlagOn()
    locals()[get_variable_name("warp", lod, "")] = vtk.vtkWarpScalar()
    locals()[get_variable_name("warp", lod, "")].SetInputConnection(locals()[get_variable_name("geom", lod, "")].GetOutputPort())
    locals()[get_variable_name("warp", lod, "")].SetNormal(0,0,1)
    locals()[get_variable_name("warp", lod, "")].UseNormalOn()
    locals()[get_variable_name("warp", lod, "")].SetScaleFactor(Scale)
    locals()[get_variable_name("warp", lod, "")].ReleaseDataFlagOn()
    locals()[get_variable_name("elevation", lod, "")] = vtk.vtkElevationFilter()
    locals()[get_variable_name("elevation", lod, "")].SetInputConnection(locals()[get_variable_name("warp", lod, "")].GetOutputPort())
    locals()[get_variable_name("elevation", lod, "")].SetLowPoint(0,0,lo)
    locals()[get_variable_name("elevation", lod, "")].SetHighPoint(0,0,hi)
    locals()[get_variable_name("elevation", lod, "")].SetScalarRange(lo,hi)
    locals()[get_variable_name("elevation", lod, "")].ReleaseDataFlagOn()
    locals()[get_variable_name("toPoly", lod, "")] = vtk.vtkCastToConcrete()
    locals()[get_variable_name("toPoly", lod, "")].SetInputConnection(locals()[get_variable_name("elevation", lod, "")].GetOutputPort())
    locals()[get_variable_name("normals", lod, "")] = vtk.vtkPolyDataNormals()
    locals()[get_variable_name("normals", lod, "")].SetInputConnection(locals()[get_variable_name("toPoly", lod, "")].GetOutputPort())
    locals()[get_variable_name("normals", lod, "")].SetFeatureAngle(60)
    locals()[get_variable_name("normals", lod, "")].ConsistencyOff()
    locals()[get_variable_name("normals", lod, "")].SplittingOff()
    locals()[get_variable_name("normals", lod, "")].ReleaseDataFlagOn()
    locals()[get_variable_name("demMapper", lod, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("demMapper", lod, "")].SetInputConnection(locals()[get_variable_name("normals", lod, "")].GetOutputPort())
    locals()[get_variable_name("demMapper", lod, "")].SetScalarRange(lo,hi)
    locals()[get_variable_name("demMapper", lod, "")].SetLookupTable(lut)
    locals()[get_variable_name("demMapper", lod, "")].ImmediateModeRenderingOn()
    locals()[get_variable_name("demMapper", lod, "")].Update()
    demActor.AddLODMapper(locals()[get_variable_name("demMapper", lod, "")])

    pass
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(demActor)
ren1.SetBackground(.4,.4,.4)
iren.SetDesiredUpdateRate(1)
def TkCheckAbort (__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent",TkCheckAbort)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.GetActiveCamera().SetPosition(-99900,-21354,131801)
ren1.GetActiveCamera().SetFocalPoint(41461,41461,2815)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.Render()
# --- end of script --
