#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# The dataset read by this exercise ("combVectors.vtk") has field data
# associated with the pointdata, namely two vector fields. In this exercise,
# you will convert both sets of field data into attribute data. Mappers only
# process attribute data, not field data. So we must convert the field data to
# attribute data in order to display it.  (You'll need to determine the "names"
# of the two vector fields in the field data.)
#
# If there is time remaining, you might consider adding a programmable filter
# to convert the two sets of vectors into a single scalar field, representing
# the angle between the two vector fields.
#
# You will most likely use vtkFieldDataToAttributeDataFilter, vtkHedgeHog,
# and vtkProgrammableAttributeDataFilter.
#
# Create the RenderWindow, Renderer and interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
# get the pressure gradient vector field
pl3d_gradient = vtk.vtkMultiBlockPLOT3DReader()
pl3d_gradient.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d_gradient.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
pl3d_gradient.SetScalarFunctionNumber(100)
pl3d_gradient.SetVectorFunctionNumber(210)
pl3d_gradient.Update()
pl3d_g_output = pl3d_gradient.GetOutput().GetBlock(0)
# get the velocity vector field
pl3d_velocity = vtk.vtkMultiBlockPLOT3DReader()
pl3d_velocity.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d_velocity.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
pl3d_velocity.SetScalarFunctionNumber(100)
pl3d_velocity.SetVectorFunctionNumber(200)
pl3d_velocity.Update()
pl3d_v_output = pl3d_velocity.GetOutput().GetBlock(0)
# contour the scalar fields
contour = vtk.vtkContourFilter()
contour.SetInputData(pl3d_g_output)
contour.SetValue(0,0.225)
# probe the vector fields to get data at the contour surface
probe_gradient = vtk.vtkProbeFilter()
probe_gradient.SetInputConnection(contour.GetOutputPort())
probe_gradient.SetSourceData(pl3d_g_output)
probe_velocity = vtk.vtkProbeFilter()
probe_velocity.SetInputConnection(contour.GetOutputPort())
probe_velocity.SetSourceData(pl3d_v_output)
#
# To display the vector fields, we use vtkHedgeHog to create lines.
#
velocity = vtk.vtkHedgeHog()
velocity.SetInputConnection(probe_velocity.GetOutputPort())
velocity.SetScaleFactor(0.0015)
pressureGradient = vtk.vtkHedgeHog()
pressureGradient.SetInputConnection(probe_gradient.GetOutputPort())
pressureGradient.SetScaleFactor(0.00002)
def ExecuteDot (__vtk__temp0=0,__vtk__temp1=0):
    # proc for ProgrammableAttributeDataFilter.  Note the use of "double()"
    # in the calculations.  This protects us from Tcl using ints and
    # overflowing.
    inputs = dotProduct.GetInputList()
    input0 = inputs.GetDataSet(0)
    input1 = inputs.GetDataSet(1)
    numPts = input0.GetNumberOfPoints()
    vectors0 = input0.GetPointData().GetVectors()
    vectors1 = input1.GetPointData().GetVectors()
    scalars = vtk.vtkFloatArray()
    i = 0
    while i < numPts:
        v0 = vectors0.GetTuple3(i)
        v1 = vectors1.GetTuple3(i)
        v0x = lindex(v0,0)
        v0y = lindex(v0,1)
        v0z = lindex(v0,2)
        v1x = lindex(v1,0)
        v1y = lindex(v1,1)
        v1z = lindex(v1,2)
        l0 = expr.expr(globals(), locals(),["double","(","v0x",")*","double","(","v0x",")","+","double","(","v0y",")*","double","(","v0y",")","+","double","(","v0z",")*","double","(","v0z",")"])
        l1 = expr.expr(globals(), locals(),["double","(","v1x",")*","double","(","v1x",")","+","double","(","v1y",")*","double","(","v1y",")","+","double","(","v1z",")*","double","(","v1z",")"])
        l0 = expr.expr(globals(), locals(),["sqrt","(","double","(","l0","))"])
        l1 = expr.expr(globals(), locals(),["sqrt","(","double","(","l1","))"])
        if (l0 > 0.0 and l1 > 0.0):
            d = expr.expr(globals(), locals(),["(","double","(","v0x",")*","double","(","v1x",")","+","double","(","v0y",")*","double","(","v1y",")","+","double","(","v0z",")*","double","(","v1z","))/(","l0","*","l1",")"])
            pass
        else:
            d = 0.0
            pass
        scalars.InsertValue(i,d)
        i = i + 1

    dotProduct.GetOutput().GetPointData().SetScalars(scalars)
    del scalars

#
# We use the ProgrammableAttributeDataFilter to compute the cosine
# of the angle between the two vector fields (i.e. the dot product
# normalized by the product of the vector lengths).
#
#
dotProduct = vtk.vtkProgrammableAttributeDataFilter()
dotProduct.SetInputConnection(probe_velocity.GetOutputPort())
dotProduct.AddInput(probe_velocity.GetOutput())
dotProduct.AddInput(probe_gradient.GetOutput())
dotProduct.SetExecuteMethod(ExecuteDot)
#
# Create the mappers and actors.  Note the call to GetPolyDataOutput when
# setting up the mapper for the ProgrammableAttributeDataFilter
#
velocityMapper = vtk.vtkPolyDataMapper()
velocityMapper.SetInputConnection(velocity.GetOutputPort())
velocityMapper.ScalarVisibilityOff()
velocityActor = vtk.vtkLODActor()
velocityActor.SetMapper(velocityMapper)
velocityActor.SetNumberOfCloudPoints(1000)
velocityActor.GetProperty().SetColor(1,0,0)
pressureGradientMapper = vtk.vtkPolyDataMapper()
pressureGradientMapper.SetInputConnection(pressureGradient.GetOutputPort())
pressureGradientMapper.ScalarVisibilityOff()
pressureGradientActor = vtk.vtkLODActor()
pressureGradientActor.SetMapper(pressureGradientMapper)
pressureGradientActor.SetNumberOfCloudPoints(1000)
pressureGradientActor.GetProperty().SetColor(0,1,0)
dotMapper = vtk.vtkPolyDataMapper()
dotMapper.SetInputConnection(dotProduct.GetOutputPort())
dotMapper.SetScalarRange(-1,1)
dotActor = vtk.vtkLODActor()
dotActor.SetMapper(dotMapper)
dotActor.SetNumberOfCloudPoints(1000)
#
# The PLOT3DReader is used to draw the outline of the original dataset.
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
#
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(velocityActor)
ren1.AddActor(pressureGradientActor)
ren1.AddActor(dotActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,500)
#ren1 SetBackground 0.1 0.2 0.4
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(-21.6807,-22.6387,35.9759)
cam1.SetViewUp(-0.0158865,0.293715,0.955761)
# render the image
#
renWin.Render()
renWin.SetWindowName("Multidimensional Visualization Exercise")
# prevent the tk window from showing up then start the event loop
# --- end of script --
