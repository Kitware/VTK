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
# vtkpython TestFreetypeTextMapper.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Rendering

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestFreetypeTextMapperBigger(vtk.test.Testing.vtkTest):

    def testFreetypeTextMapperBigger(self):

        currentFontSize = 55
        defaultText = "MmNnKk @"
        textColor = [246, 255, 11]
        bgColor = [56, 56, 154]
        for i in range(0, len(textColor)):
            textColor[i] /= 255.0
            bgColor[i] /= 255.0

        renWin = vtk.vtkRenderWindow()
        renWin.SetSize(790, 450)

        ren = vtk.vtkRenderer()
        ren.SetBackground(bgColor)
        renWin.AddRenderer(ren)

        families = ["Arial", "Courier", "Times"]
        attributes = [[0, 0], [1, 1]] # bold, italic

        def SetAttributesText(attrib):
            """ Expects a list of attributes of size 2, returns a string  """
            s = ""
            if attrib[0] != 0:
                s += "b"
            if attrib[1] != 0:
                s += "i"
            return ','.join(list(s))

        mapper = dict()
        actor = dict()

        pos = 0
        for i, family in enumerate(families):
            for j, attrib in enumerate(attributes):
                pos += 1
                txt = ""
                txtAttrib = SetAttributesText(attrib)
                if len(txtAttrib) != 0:
                    txt = family + " (" + SetAttributesText(attrib) + "): " + defaultText
                else:
                    txt = family + ": " + defaultText

                idx = ''.join(map(str, [i, j]))
                mapper.update({idx:vtk.vtkOpenGLFreeTypeTextMapper()})
                mapper[idx].SetInput(txt)

                tprop = mapper[idx].GetTextProperty()
                eval('tprop.SetFontFamilyTo' + family + '()')
                tprop.SetColor(textColor)
                tprop.SetBold(attrib[0])
                tprop.SetItalic(attrib[1])
                tprop.SetFontSize(currentFontSize)

                actor.update({idx:vtk.vtkActor2D()})
                actor[idx].SetMapper(mapper[idx])
                actor[idx].SetDisplayPosition(10, pos * (currentFontSize + 5))

                ren.AddActor(actor[idx])

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        renWin.Render()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestFreetypeTextMapperBigger, 'test')])
