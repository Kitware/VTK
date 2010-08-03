#!/usr/bin/env python

import sys
import os
import PyQt4

import vtk
from vtk.test import Testing

class TestTimeUtility(Testing.vtkTest):
  def testConvertBack(self):
    t = 0
    d = vtk.vtkQtTimePointUtility.TimePointToQDateTime(t)
    print d
    t2 = vtk.vtkQtTimePointUtility.QDateTimeToTimePoint(d)
    print t2
    self.assertEqual(t, t2)

  def testConvertBack2(self):
    t = 0
    t2 = vtk.vtkQtTimePointUtility.QDateTimeToTimePoint(vtk.vtkQtTimePointUtility.TimePointToQDateTime(t))
    self.assertEqual(t, t2)

if __name__ == "__main__":
  Testing.main([(TestTimeUtility, 'test')])
