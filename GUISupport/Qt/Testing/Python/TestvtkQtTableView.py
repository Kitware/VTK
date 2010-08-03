#!/usr/bin/env python

import sys
import os
import PyQt4

import vtk
from vtk.test import Testing

class TestvtkQtTableView(Testing.vtkTest):
  def testvtkQtTableView(self):

    sphereSource = vtk.vtkSphereSource()
    tableConverter = vtk.vtkDataObjectToTable()
    tableConverter.SetInput(sphereSource.GetOutput())
    tableConverter.SetFieldType(1)
    tableConverter.Update()
    pointTable = tableConverter.GetOutput();

    tableView = vtk.vtkQtTableView()
    tableView.SetSplitMultiComponentColumns(1);
    tableView.AddRepresentationFromInput(pointTable);
    tableView.Update();
    w = tableView.GetWidget()
    w.show();

    if Testing.isInteractive():
      PyQt4.QtGui.qApp.exec_()


if __name__ == "__main__":
  app = PyQt4.QtGui.QApplication(sys.argv)
  Testing.main([(TestvtkQtTableView, 'test')])
