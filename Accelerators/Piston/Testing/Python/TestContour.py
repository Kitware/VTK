#!/usr/bin/env python
"""
This tests vtk's use of Piston's marching_cube operator.

"""

import sys
import vtk

from vtk.test import Testing
from PistonTestCommon import *

class TestContour(Testing.vtkTest):
  def testContour(self):
    global args
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
    src.SetWholeExtent(0,10,0,10,0,10)

    #scale and bias until piston understands origin and spacing
    src.Update()
    inputdata = src.GetOutput()
    if "Normalize" in args:
         testdata1 = inputdata.NewInstance()
         testdata1.ShallowCopy(inputdata)
         testdata1.SetSpacing(1,1,1)
         testdata1.SetOrigin(0,0,0)
         inputdata = testdata1

    d2p = vtk.vtkDataSetToPiston()
    d2p.SetInputData(inputdata)
    #d2p.SetInputConnection(src.GetOutputPort())

    filter = vtk.vtkPistonContour()
    filter.SetInputConnection(d2p.GetOutputPort())
    filter.SetIsoValue(50)

    p2d = vtk.vtkPistonToDataSet()
    p2d.SetOutputDataSetType(vtk.VTK_POLY_DATA)
    p2d.SetInputConnection(filter.GetOutputPort())

    if writefiles:
        writeFile(p2d, "piston_contour.vtk")

    mapper = vtk.vtkPistonMapper()
    mapper.SetInputConnection(filter.GetOutputPort())
    mapper.Update() #TODO MTIMES ARE MAKING THIS NECESSARY, WHY?

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    renderer.AddActor(actor)
    renderer.ResetCamera()
    renwin.Render()

    img_file = "TestContour.png"
    #Testing.compareImage(renwin, Testing.getAbsImagePath(img_file))

    if Testing.isInteractive():
        iren.Start()

if __name__ == "__main__":
    global args
    args = parseArgs()
    Testing.main([(TestContour, 'test')])
