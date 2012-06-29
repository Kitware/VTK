#!/usr/bin/env python
"""
This tests the ability to render GPU resident data in VTK.

"""

import sys
import vtk

from vtk.test import Testing
from PistonTestCommon import *

class TestRendering(Testing.vtkTest):
  def testRendering(self):
    global args

    renderer = vtk.vtkRenderer()
    renwin = vtk.vtkRenderWindow()
    renwin.AddRenderer(renderer)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renwin)
    renwin.Render()

    if "GPURender" in args:
        print "Testing GPU direct render path"
        vtk.vtkPistonMapper.InitCudaGL(renwin)
    else:
        print "Testing CPU indirect render path"

    src = vtk.vtkSphereSource()

    d2p = vtk.vtkDataSetToPiston()
    d2p.SetInputConnection(src.GetOutputPort())

    mapper = vtk.vtkPistonMapper()
    mapper.SetInputConnection(d2p.GetOutputPort())
    mapper.Update() #TODO: shouldn't need this

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    renderer.AddActor(actor)

    renderer.ResetCamera()
    renwin.Render()

    img_file = "TestRendering.png"
    Testing.compareImage(renwin, Testing.getAbsImagePath(img_file))

    if Testing.isInteractive():
        iren.Start()

if __name__ == "__main__":
    global args
    args = parseArgs()
    Testing.main([(TestRendering, 'test')])
