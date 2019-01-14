#!/usr/bin/env python

import sys
import os
import PyQt4

import vtk

class TestQVTKWidget(Testing.vtkTest):
  def testQVTKWidget2(self):

    w2 = vtk.QVTKWidget2()
    w2.resize(500,500)

    ren = vtk.vtkRenderer()
    ren.SetBackground(0,0,0)
    ren.SetBackground2(1,1,1)
    ren.SetGradientBackground(1)
    win2 = vtk.vtkGenericOpenGLRenderWindow()
    win2.AddRenderer(ren)
    w2.SetRenderWindow(win2)

    renwin = w2.GetRenderWindow()
    cone = vtk.vtkConeSource()
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInput(cone.GetOutput())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    ren.AddViewProp(actor)
    ren.ResetCamera()

    w2.show()
    if Testing.isInteractive():
      PyQt4.QtGui.qApp.exec_()

if __name__ == "__main__":
  app = PyQt4.QtGui.QApplication(sys.argv)
  Testing.main([(TestQVTKWidget, 'test')])
