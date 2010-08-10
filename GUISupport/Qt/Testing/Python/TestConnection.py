#!/usr/bin/env python

import sys
import os
import PyQt4

import vtk
from vtk.test import Testing

class TestConnectionHelper(PyQt4.QtCore.QObject):
  @PyQt4.QtCore.pyqtSlot()
  def myslot(self):
    self.triggered = 1;

class TestConnection(Testing.vtkTest):
  def test_connection(self):
    renWin = vtk.vtkRenderWindow()
    c = vtk.vtkEventQtSlotConnect()
    t = TestConnectionHelper()
    c.Connect(renWin, 3, t, PyQt4.QtCore.SLOT("myslot()"), "", 0, PyQt4.QtCore.Qt.AutoConnection)
    renWin.Render()
    self.assertEqual(1, t.triggered)

if __name__ == "__main__":
  app = PyQt4.QtGui.QApplication(sys.argv)
  Testing.main([(TestConnection, 'test')])
