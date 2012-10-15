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
# Generate some random colors
def MakeColors (lut,n,__vtk__temp0=0,__vtk__temp1=0):
    catch.catch(globals(),"""math = vtk.vtkMath()""")
    lut.SetNumberOfColors(n)
    lut.SetTableRange(0,expr.expr(globals(), locals(),["n","-1"]))
    lut.SetScaleToLinear()
    lut.Build()
    lut.SetTableValue(0,0,0,0,1)
    math.RandomSeed(5071)
    i = 1
    while i < n:
        lut.SetTableValue(i,math.Random(.2,1),math.Random(.2,1),math.Random(.2,1),1)
        i = i + 1


lut = vtk.vtkLookupTable()
MakeColors(lut,256)
n = 20
radius = 10
# This has been moved outside the loop so that the code can be correctly
# translated to python
catch.catch(globals(),"""blobImage = vtk.vtkImageData()""")
i = 0
while i < n:
    catch.catch(globals(),"""sphere = vtk.vtkSphere()""")
    sphere.SetRadius(radius)
    max = expr.expr(globals(), locals(),["50","-","radius"])
    sphere.SetCenter(expr.expr(globals(), locals(),["int","(","math.Random(-max,max)",")"]),expr.expr(globals(), locals(),["int","(","math.Random(-max,max)",")"]),expr.expr(globals(), locals(),["int","(","math.Random(-max,max)",")"]))
    catch.catch(globals(),"""sampler = vtk.vtkSampleFunction()""")
    sampler.SetImplicitFunction(sphere)
    sampler.SetOutputScalarTypeToFloat()
    sampler.SetSampleDimensions(51,51,51)
    sampler.SetModelBounds(-50,50,-50,50,-50,50)
    catch.catch(globals(),"""thres = vtk.vtkImageThreshold()""")
    thres.SetInputConnection(sampler.GetOutputPort())
    thres.ThresholdByLower(expr.expr(globals(), locals(),["radius","*","radius"]))
    thres.ReplaceInOn()
    thres.ReplaceOutOn()
    thres.SetInValue(expr.expr(globals(), locals(),["i","+","1"]))
    thres.SetOutValue(0)
    thres.Update()
    if (i == 0):
        blobImage.DeepCopy(thres.GetOutput())
        pass
    catch.catch(globals(),"""maxValue = vtk.vtkImageMathematics()""")
    maxValue.SetInputData(0,blobImage)
    maxValue.SetInputData(1,thres.GetOutput())
    maxValue.SetOperationToMax()
    maxValue.Modified()
    maxValue.Update()
    blobImage.DeepCopy(maxValue.GetOutput())
    i = i + 1

discrete = vtk.vtkDiscreteMarchingCubes()
discrete.SetInputData(blobImage)
discrete.GenerateValues(n,1,n)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(discrete.GetOutputPort())
mapper.SetLookupTable(lut)
mapper.SetScalarRange(0,lut.GetNumberOfColors())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
