#!/usr/bin/env python

# This example demonstrates the use of fields and use of
# vtkProgrammableDataObjectSource. It creates fields the hard way (as
# compared to reading a vtk field file), but shows you how to
# interface to your own raw data.

import os
import re
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

xAxis = "INTEREST_RATE"
yAxis = "MONTHLY_PAYMENT"
zAxis = "MONTHLY_INCOME"
scalar = "TIME_LATE"

def getNumberFromLine(line):
    patn = re.compile('[-+]{0,1}[\d.]+e?[-+\d]*', re.M)
    val = patn.findall(line)
    ret = []
    for i in val:
        ret.append(float(i))
    return ret

# Parse an ASCII file and manually create a field. Then construct a 
# dataset from the field.
dos = vtk.vtkProgrammableDataObjectSource()

# First define the function that will parse the data.
def parseFile():
    global VTK_DATA_ROOT, dos

    # Use Python to read an ASCII file
    file = open(os.path.join(VTK_DATA_ROOT, "Data/financial.txt"), "r")

    line = file.readline()
    numPts = int(getNumberFromLine(line)[0])

    numLines = (numPts - 1)/8
    # Get the data object's field data and allocate
    # room for 4, fields
    fieldData = dos.GetOutput().GetFieldData()
    fieldData.AllocateArrays(4)

    # read TIME_LATE - dependent variable
    # search the file until an array called TIME_LATE is found
    while file.readline()[:9] != "TIME_LATE":
        pass

    # Create the corresponding float array
    timeLate = vtk.vtkFloatArray()
    timeLate.SetName("TIME_LATE")
    # Read the values
    for i in range(0, numLines):
        val = getNumberFromLine(file.readline())
        for j in range(0, 8):
            timeLate.InsertNextValue(val[j])            
    # Add the array
    fieldData.AddArray(timeLate)
    
    # MONTHLY_PAYMENT - independent variable
    while file.readline()[:15] != "MONTHLY_PAYMENT":
        pass

    monthlyPayment = vtk.vtkFloatArray()
    monthlyPayment.SetName("MONTHLY_PAYMENT")
    for i in range(0, numLines):
        val = getNumberFromLine(file.readline())
        for j in range(0, 8):
            monthlyPayment.InsertNextValue(val[j])
            
    fieldData.AddArray(monthlyPayment)

    # UNPAID_PRINCIPLE - skip
    while file.readline()[:16] != "UNPAID_PRINCIPLE":
        pass
    for i in range(0, numLines):
        file.readline()

    # LOAN_AMOUNT - skip
    while file.readline()[:11] != "LOAN_AMOUNT":
        pass
    for i in range(0, numLines):
        file.readline()


    # INTEREST_RATE - independent variable
    while file.readline()[:13] != "INTEREST_RATE":
        pass
    
    interestRate = vtk.vtkFloatArray()
    interestRate.SetName("INTEREST_RATE")
    for i in range(0, numLines):
        val = getNumberFromLine(file.readline())
        for j in range(0, 8):
            interestRate.InsertNextValue(val[j])
            
    fieldData.AddArray(interestRate)

    # MONTHLY_INCOME - independent variable
    while file.readline()[:14] != "MONTHLY_INCOME":
        pass
    
    monthlyIncome = vtk.vtkIntArray()
    monthlyIncome.SetName("MONTHLY_INCOME")
    for i in range(0, numLines):
        val = getNumberFromLine(file.readline())
        for j in range(0, 8):
            monthlyIncome.InsertNextValue(val[j])
            
    fieldData.AddArray(monthlyIncome)

# Arrange to call the parsing function when the programmable data
# source is executed.
dos.SetExecuteMethod(parseFile)

# Create the dataset.

# DataObjectToDataSetFilter can create geometry using fields from
# DataObject's FieldData
do2ds = vtk.vtkDataObjectToDataSetFilter()
do2ds.SetInput(dos.GetOutput())
# We are generating polygonal data
do2ds.SetDataSetTypeToPolyData()
do2ds.DefaultNormalizeOn()
# All we need is points. Assign them.
do2ds.SetPointComponent(0, xAxis, 0)
do2ds.SetPointComponent(1, yAxis, 0)
do2ds.SetPointComponent(2, zAxis, 0)

# RearrangeFields is used to move fields between DataObject's
# FieldData, PointData and CellData.
rf = vtk.vtkRearrangeFields()
rf.SetInput(do2ds.GetOutput())
# Add an operation to "move TIME_LATE from DataObject's FieldData to
# PointData"
rf.AddOperation("MOVE", scalar, "DATA_OBJECT", "POINT_DATA")
# Force the filter to execute. This is need to force the pipeline
# to execute so that we can find the range of the array TIME_LATE
rf.Update()
# Set max to the second (GetRange returns [min,max]) of the "range of the 
# array called scalar in the PointData of the output of rf"
max = rf.GetOutput().GetPointData().GetArray(scalar).GetRange()[1]


# Use an ArrayCalculator to normalize TIME_LATE
calc = vtk.vtkArrayCalculator()
calc.SetInput(rf.GetOutput())
# Working on point data
calc.SetAttributeModeToUsePointData()
# Map scalar to s. When setting function, we can use s to
# represent the array scalar (TIME_LATE) 
calc.AddScalarVariable("s", scalar, 0)
# Divide scalar by max (applies division to all components of the array)
calc.SetFunction("s / %f"%max)
# The output array will be called resArray
calc.SetResultArrayName("resArray")

# Use AssignAttribute to make resArray the active scalar field
aa = vtk.vtkAssignAttribute()
aa.SetInput(calc.GetOutput())
aa.Assign("resArray", "SCALARS", "POINT_DATA")
aa.Update()

# construct pipeline for original population
# GaussianSplatter -> Contour -> Mapper -> Actor
popSplatter = vtk.vtkGaussianSplatter()
popSplatter.SetInput(aa.GetOutput())
popSplatter.SetSampleDimensions(50, 50, 50)
popSplatter.SetRadius(0.05)
popSplatter.ScalarWarpingOff()

popSurface = vtk.vtkContourFilter()
popSurface.SetInput(popSplatter.GetOutput())
popSurface.SetValue(0, 0.01)

popMapper = vtk.vtkPolyDataMapper()
popMapper.SetInput(popSurface.GetOutput())
popMapper.ScalarVisibilityOff()

popActor = vtk.vtkActor()
popActor.SetMapper(popMapper)
popActor.GetProperty().SetOpacity(0.3)
popActor.GetProperty().SetColor(.9, .9, .9)

# This is for decoration only.
def CreateAxes():
    global xAxis, yAxis, zAxis, popSplatter

    # Create axes.
    popSplatter.Update()
    bounds = popSplatter.GetOutput().GetBounds()
    axes = vtk.vtkAxes()
    axes.SetOrigin(bounds[0], bounds[2], bounds[4])
    axes.SetScaleFactor(popSplatter.GetOutput().GetLength()/5.0)
    
    axesTubes = vtk.vtkTubeFilter()
    axesTubes.SetInput(axes.GetOutput())
    axesTubes.SetRadius(axes.GetScaleFactor()/25.0)
    axesTubes.SetNumberOfSides(6)
    
    axesMapper = vtk.vtkPolyDataMapper()
    axesMapper.SetInput(axesTubes.GetOutput())

    axesActor = vtk.vtkActor()
    axesActor.SetMapper(axesMapper)

    # Label the axes.
    XText = vtk.vtkVectorText()
    XText.SetText(xAxis)

    XTextMapper = vtk.vtkPolyDataMapper()
    XTextMapper.SetInput(XText.GetOutput())

    XActor = vtk.vtkFollower()
    XActor.SetMapper(XTextMapper)
    XActor.SetScale(0.02, .02, .02)
    XActor.SetPosition(0.35, -0.05, -0.05)
    XActor.GetProperty().SetColor(0, 0, 0)
    
    YText = vtk.vtkVectorText()
    YText.SetText(yAxis)
    
    YTextMapper = vtk.vtkPolyDataMapper()
    YTextMapper.SetInput(YText.GetOutput())
    
    YActor = vtk.vtkFollower()
    YActor.SetMapper(YTextMapper)
    YActor.SetScale(0.02, .02, .02)
    YActor.SetPosition(-0.05, 0.35, -0.05)
    YActor.GetProperty().SetColor(0, 0, 0)
    
    ZText = vtk.vtkVectorText()
    ZText.SetText(zAxis)
    
    ZTextMapper = vtk.vtkPolyDataMapper()
    ZTextMapper.SetInput(ZText.GetOutput())
    
    ZActor = vtk.vtkFollower()
    ZActor.SetMapper(ZTextMapper)
    ZActor.SetScale(0.02, .02, .02)
    ZActor.SetPosition(-0.05, -0.05, 0.35)
    ZActor.GetProperty().SetColor(0, 0, 0)
    return axesActor, XActor, YActor, ZActor
 
axesActor, XActor, YActor, ZActor = CreateAxes()

# Create the render window, renderer, interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetWindowName("vtk - Field Data")
renWin.SetSize(500, 500)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(axesActor)
ren.AddActor(XActor)
ren.AddActor(YActor)
ren.AddActor(ZActor)
ren.AddActor(popActor)
ren.SetBackground(1, 1, 1)

# Set the default camera position
camera = vtk.vtkCamera()
camera.SetClippingRange(.274, 13.72)
camera.SetFocalPoint(0.433816, 0.333131, 0.449)
camera.SetPosition(-1.96987, 1.15145, 1.49053)
camera.SetViewUp(0.378927, 0.911821, 0.158107)
ren.SetActiveCamera(camera)
# Assign the camera to the followers.
XActor.SetCamera(camera)
YActor.SetCamera(camera)
ZActor.SetCamera(camera)

iren.Initialize()
renWin.Render()
iren.Start()
