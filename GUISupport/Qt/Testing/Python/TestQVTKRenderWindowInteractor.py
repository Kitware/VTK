#!/usr/bin/env python

import sys
import os

try:
    from PyQt4 import QtCore, QtGui
except ImportError:
    try:
        from PySide import QtCore, QtGui
    except ImportError:
        raise ImportError("Cannot load either PyQt or PySide")

import vtk
from vtk.qt4.QVTKRenderWindowInteractor import *

class TestQVTKRenderWindowInteractor(Testing.vtkTest):
    def testQVTKRenderWindowInteractor(self):
        w2 = QVTKRenderWindowInteractor()
        w2.Initialize()

        ren = vtk.vtkRenderer()
        ren.SetBackground(0,0,0)
        ren.SetBackground2(1,1,1)
        ren.SetGradientBackground(1)

        renwin = w2.GetRenderWindow()
        renwin.AddRenderer(ren)
        cone = vtk.vtkConeSource()
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(cone.GetOutputPort())
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        ren.AddViewProp(actor)
        ren.ResetCamera()

        w2.show()
        if Testing.isInteractive():
            QtGui.qApp.exec_()

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    Testing.main([(TestQVTKRenderWindowInteractor, 'test')])
