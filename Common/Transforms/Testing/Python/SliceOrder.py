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

import vtk

class SliceOrder(object):
    '''
        These transformations permute medical image data to maintain proper
        orientation regardless of the acqusition order.
        After applying these transforms with vtkTransformFilter,
        a view up of 0,-1,0 will result in the body part
        facing the viewer.
        NOTE: some transformations have a -1 scale factor
              for one of the components.
              To ensure proper polygon orientation and normal direction,
              you must apply the vtkPolyDataNormals filter.

        Naming:
        si - superior to inferior (top to bottom)
        iss - inferior to superior (bottom to top)
        ap - anterior to posterior (front to back)
        pa - posterior to anterior (back to front)
        lr - left to right
        rl - right to left

    '''

    si = vtk.vtkTransform()
    si.SetMatrix([1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1])

    # is is a reserved word in Python so use iss
    iss = vtk.vtkTransform()
    iss.SetMatrix([1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 1])

    ap = vtk.vtkTransform()
    ap.Scale(1, -1, 1)

    pa = vtk.vtkTransform()
    pa.Scale(1, -1, -1)

    lr = vtk.vtkTransform()
    lr.SetMatrix([0, 0, -1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1])

    rl = vtk.vtkTransform()
    rl.SetMatrix([0, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1])

    #
    # the previous transforms assume radiological views of the slices
    # (viewed from the feet).
    #  Othermodalities such as physical sectioning may view from the head.
    # These transforms modify the original with a 180 rotation about y
    #
    hf = vtk.vtkTransform()
    hf.SetMatrix([-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1])

    hfsi = vtk.vtkTransform()
    hfsi.Concatenate(hf.GetMatrix())
    hfsi.Concatenate(si.GetMatrix())

    hfis = vtk.vtkTransform()
    hfis.Concatenate(hf.GetMatrix())
    hfis.Concatenate(iss.GetMatrix())

    hfap = vtk.vtkTransform()
    hfap.Concatenate(hf.GetMatrix())
    hfap.Concatenate(ap.GetMatrix())

    hfpa = vtk.vtkTransform()
    hfpa.Concatenate(hf.GetMatrix())
    hfpa.Concatenate(pa.GetMatrix())

    hflr = vtk.vtkTransform()
    hflr.Concatenate(hf.GetMatrix())
    hflr.Concatenate(lr.GetMatrix())

    hfrl = vtk.vtkTransform()
    hfrl.Concatenate(hf.GetMatrix())
    hfrl.Concatenate(rl.GetMatrix())
