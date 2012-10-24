#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

# Run this test like so:
# vtkpython TestSortFileNames.py  -D $VTK_DATA_ROOT

import re
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestSortFileNames(vtk.test.Testing.vtkTest):

    def testSortFileNames(self):

        globFileNames = vtk.vtkGlobFileNames()

        # globs do not include Kleene star support for patern repetitions thus
        # we insert a pattern for both single and double digit file extensions.
        globFileNames.AddFileNames(VTK_DATA_ROOT + "/Data/headsq/quarter.[1-9]")
        globFileNames.AddFileNames(VTK_DATA_ROOT + "/Data/headsq/quarter.[1-9][0-9]")
        globFileNames.AddFileNames(VTK_DATA_ROOT + "/Data/track*.binary.vtk")

        sortFileNames = vtk.vtkSortFileNames()
        sortFileNames.SetInputFileNames(globFileNames.GetFileNames())
        sortFileNames.NumericSortOn()
        sortFileNames.SkipDirectoriesOn()
        sortFileNames.IgnoreCaseOn()
        sortFileNames.GroupingOn()

        if sortFileNames.GetNumberOfGroups() != 2:
            print "GetNumberOfGroups returned incorrect number"
            exit(1)

        fileNames1 = sortFileNames.GetNthGroup(0)
        fileNames2 = sortFileNames.GetNthGroup(1)

        numberOfFiles1 = 93
        numberOfFiles2 = 3

        n = fileNames1.GetNumberOfValues()
        if n != numberOfFiles1:
            for i in range(0, n):
                print fileNames1.GetValue(i)
            print "GetNumberOfValues should return", numberOfFiles1, "not", n
            exit(1)

        for i in range(0, numberOfFiles1):
            j = i + 1
            s = VTK_DATA_ROOT + "/Data/headsq/quarter." + str(j)
            if fileNames1.GetValue(i) != s:
                print "string does not match pattern"
                print fileNames1.GetValue(i)
                print s
                exit(1)

        n = fileNames2.GetNumberOfValues()
        if n != numberOfFiles2:
            for i in range(0, n):
                print fileNames2.GetValue(i)
            print "GetNumberOfValues should return", numberOfFiles2, "not", n
            exit(1)

        for i in range(0, numberOfFiles2):
            j = i + 1
            s = VTK_DATA_ROOT + "/Data/track" + str(j) + ".binary.vtk"
            if fileNames2.GetValue(i) != s:
                print "string does not match pattern"
                print fileNames2.GetValue(i)
                print s
                exit(1)

        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestSortFileNames, 'test')])
