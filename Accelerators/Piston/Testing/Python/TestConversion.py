#!/usr/bin/env python
"""
This tests the ability to send data back and forth between the CPU and GPU.

"""

import sys
import vtk

from vtk.test import Testing
from PistonTestCommon import *

class TestConversion(Testing.vtkTest):
  def testConversion(self):
    global args
    writefiles = "SaveData" in args

    src = vtk.vtkImageMandelbrotSource()
    src2 = vtk.vtkSphereSource()
    d2p = vtk.vtkDataSetToPiston()
    p2d = vtk.vtkPistonToDataSet()

    #test sending imagedata to the card and getting it back
    src.SetWholeExtent(0,10,0,10,0,10)
    d2p.SetInputConnection(src.GetOutputPort())
    p2d.SetOutputDataSetType(vtk.VTK_IMAGE_DATA)
    p2d.SetInputConnection(d2p.GetOutputPort())
    p2d.Update()

    printDS("INPUT IMAGE DATA", src.GetOutput())
    printTDO("GPU VERSION", d2p.GetPistonDataObjectOutput(0))
    printDS("OUTPUT IMAGE DATA", p2d.GetDataSetOutput(0))

    if writefiles:
        writeFile(p2d, "piston_imagedata.vtk")
    inDS = src.GetOutput()
    outDS = p2d.GetDataSetOutput(0)
    if inDS.GetBounds() != outDS.GetBounds():
        self.fail("Problem: bounds were not preserved.")
    if inDS.GetNumberOfCells() != outDS.GetNumberOfCells():
        self.fail("Problem: cells were not preserved.")
    if inDS.GetPointData().GetArray(0).GetName() != outDS.GetPointData().GetArray(0).GetName():
        self.fail("Problem: arrays were not preserved.")

    #test sending polydata to the card and getting it back
    #test a pipeline that works on poly data

    #below doesn't work in vtk6, gets null output for p2d
    #since data type AND upstream pipeline changes
    #workaround until that is fixes
    #d2p.SetInputConnection(src2.GetOutputPort())
    #p2d.SetOutputDataSetType(vtk.VTK_POLY_DATA)
    #p2d.Update()
    #printDS("INPUT POLYDATA", src2.GetOutput())
    #printTDO("GPU VERSION", d2p.GetPistonDataObjectOutput(0))
    #printDS("OUTPUT POLYDATA", p2d.GetDataSetOutput(0))
    #if writefiles:
    #    writeFile(p2d, "piston_polydata.vtk")
    #
    #so use this instead
    d2p2 = vtk.vtkDataSetToPiston()
    p2d2 = vtk.vtkPistonToDataSet()
    d2p2.SetInputConnection(src2.GetOutputPort())
    p2d2.SetOutputDataSetType(vtk.VTK_POLY_DATA)
    p2d2.SetInputConnection(d2p2.GetOutputPort())
    p2d2.Update()
    printDS("INPUT POLYDATA", src2.GetOutput())
    printTDO("GPU VERSION", d2p2.GetPistonDataObjectOutput(0))
    printDS("OUTPUT POLYDATA", p2d2.GetDataSetOutput(0))
    if writefiles:
        writeFile(p2d2, "piston_polydata.vtk")

    inDS = src2.GetOutput()
    outDS = p2d2.GetDataSetOutput(0)
    if inDS.GetBounds() != outDS.GetBounds():
        self.fail("Problem: bounds were not preserved.")
    if inDS.GetNumberOfCells() != outDS.GetNumberOfCells():
        self.fail("Problem: cells were not preserved.")
    if inDS.GetPointData().GetArray(0).GetName() != outDS.GetPointData().GetArray(0).GetName():
        self.fail("Problem: arrays were not preserved.")


if __name__ == "__main__":
    global args
    args = parseArgs()
    Testing.main([(TestConversion, 'test')])
