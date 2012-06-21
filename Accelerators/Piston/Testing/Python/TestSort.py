#!/usr/bin/env python
"""
This tests a pipeline that consists of a chain of GPU resident operators.

The data, once on the GPU, should stay there until brought back with the
vtkPistonMapper or vtkPistonToDataSet algorithms.

"""

import sys
import vtk

from vtk.test import Testing
from PistonTestCommon import *

class TestSort(Testing.vtkTest):
  def testSort(self):
    global args

    writefiles = "SaveData" in args

    renderer = vtk.vtkRenderer()
    renwin = vtk.vtkRenderWindow()
    renwin.AddRenderer(renderer)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renwin)
    renwin.Render()

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

    sorterF = vtk.vtkPistonSort()
    sorterF.SetInputConnection(d2p.GetOutputPort())

    contourF = vtk.vtkPistonContour()
    contourF.SetInputConnection(sorterF.GetOutputPort())
    contourF.SetIsoValue(50.0)

    p2d = vtk.vtkPistonToDataSet()
    p2d.SetInputConnection(contourF.GetOutputPort())
    p2d.SetOutputDataSetType(vtk.VTK_POLY_DATA)
    p2d.Update()

    if writefiles:
        writeFile(p2d, "piston_sortcontour.vtk")

    mapper = vtk.vtkDataSetMapper()
    mapper.SetInputConnection(p2d.GetOutputPort())
    mapper.Update()

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    renderer.AddActor(actor)

    renderer.ResetCamera()
    renwin.Render()

    img_file = "TestSort.png"
    Testing.compareImage(renwin, Testing.getAbsImagePath(img_file))

    if Testing.isInteractive():
        iren.Start()

if __name__ == "__main__":
    global args
    args = parseArgs()
    Testing.main([(TestSort, 'test')])
