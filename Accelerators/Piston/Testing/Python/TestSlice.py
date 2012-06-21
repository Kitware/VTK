#!/usr/bin/env python
"""
This tests VTK's use of Piston's slice operator.

"""

import sys
import vtk

from vtk.test import Testing
from PistonTestCommon import *

def widgetCallBack(obj,event):
     global plane, filter
     obj.GetPlane(plane)
     filter.Update() #TODO: Why is this necessary?

class TestSlice(Testing.vtkTest):
  def testSlice(self):
    global plane, filter, args
    writefiles = "SaveData" in args

    renderer = vtk.vtkRenderer()
    renwin = vtk.vtkRenderWindow()
    renwin.AddRenderer(renderer)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renwin)
    renwin.Render()

    if "GPURender" in args:
        vtk.vtkPistonMapper.InitCUDAGL(renwin)

    src = vtk.vtkImageMandelbrotSource()
    src.SetWholeExtent(0,20,0,20,0,20)

    #scale and bias until piston understands origin and spacing
    src.Update()
    inputdata = src.GetOutput()
    if "Normalize" in args:
         testdata1 = inputdata.NewInstance()
         testdata1.ShallowCopy(inputdata)
         testdata1.SetSpacing(1,1,1)
         testdata1.SetOrigin(0,0,0)
         inputdata = testdata1

    bounds = inputdata.GetBounds()
    center = [(bounds[1]-bounds[0])/2+bounds[0],
              (bounds[3]-bounds[2])/2+bounds[2],
              (bounds[5]-bounds[4])/2+bounds[4]]

    d2p = vtk.vtkDataSetToPiston()
    d2p.SetInputData(inputdata)
    #d2p.SetInputConnection(src.GetOutputPort())

    plane = vtk.vtkPlane()
    plane.SetOrigin(center)
    plane.SetNormal(0,0,1)

    filter = vtk.vtkPistonSlice()
    filter.SetInputConnection(d2p.GetOutputPort())
    filter.SetClippingPlane(plane)
    filter.SetOffset(0.0)

    p2d = vtk.vtkPistonToDataSet()
    p2d.SetOutputDataSetType(vtk.VTK_POLY_DATA)
    p2d.SetInputConnection(filter.GetOutputPort())
    if writefiles:
        writeFile(p2d, "piston_slice.vtk")

    mapper = vtk.vtkPistonMapper()
    mapper.SetInputConnection(filter.GetOutputPort())
    mapper.Update() #TODO why is this necessary

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    renderer.AddActor(actor)

    widget = vtk.vtkImplicitPlaneWidget()
    widget.PlaceWidget(bounds)
    widget.SetOrigin([plane.GetOrigin()[x] for x in 0,1,2])
    widget.SetNormal([plane.GetNormal()[x] for x in 0,1,2])
    widget.SetInteractor(iren)
    widget.AddObserver("InteractionEvent", widgetCallBack)
    widget.SetEnabled(1)
    widget.DrawPlaneOff()

    renderer.ResetCamera()
    renwin.Render()

    img_file = "TestSlice.png"
    Testing.compareImage(renwin, Testing.getAbsImagePath(img_file))

    if Testing.isInteractive():
        widget.DrawPlaneOn()
        iren.Start()

if __name__ == "__main__":
    global args
    args = parseArgs()
    Testing.main([(TestSlice, 'test')])
