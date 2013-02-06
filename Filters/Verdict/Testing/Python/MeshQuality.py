#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Translated from MeshQuality.cxx

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    MeshQuality.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''


import math

import vtk
from vtk.util.misc import vtkGetDataRoot

fname = vtkGetDataRoot() + '/Data/uGridEx.vtk'


def DumpQualityStats(iq, arrayname):
    an = iq.GetOutput().GetFieldData().GetArray(arrayname)
    cardinality = an.GetComponent(0, 4)
    range = list()
    range.append(an.GetComponent(0, 0))
    range.append(an.GetComponent(0, 2))
    average = an.GetComponent(0, 1)
    stdDev = math.sqrt(math.fabs(an.GetComponent(0, 3)))
    outStr = '%s%g%s%g%s%g\n%s%g%s%g' % (
            '  cardinality: ', cardinality,
            '  , range: ', range[0], '  -  ', range[1],
            '  average: ', average, '  , standard deviation: ', stdDev)
    return outStr

def main():
    mr = vtk.vtkUnstructuredGridReader()
    iq = vtk.vtkMeshQuality()

    mr.SetFileName(fname)
    mr.Update()

    ug = mr.GetOutput()
    iq.SetInputConnection(mr.GetOutputPort())

    # Here we define the various mesh types and labels for output.
    meshTypes = [['Triangle', 'Triangle',
                  [['QualityMeasureToEdgeRatio', ' Edge Ratio:'],
                   ['QualityMeasureToAspectRatio', ' Aspect Ratio:'],
                   ['QualityMeasureToRadiusRatio', ' Radius Ratio:'],
                   ['QualityMeasureToAspectFrobenius', ' Frobenius Norm:'],
                   ['QualityMeasureToMinAngle', ' Minimal Angle:']
                   ]
                  ],

                 ['Quad', 'Quadrilateral',
                  [['QualityMeasureToEdgeRatio', ' Edge Ratio:'],
                   ['QualityMeasureToAspectRatio', ' Aspect Ratio:'],
                   ['QualityMeasureToRadiusRatio', ' Radius Ratio:'],
                   ['QualityMeasureToMedAspectFrobenius',
                   ' Average Frobenius Norm:'],
                   ['QualityMeasureToMaxAspectFrobenius',
                   ' Maximal Frobenius Norm:'],
                   ['QualityMeasureToMinAngle', ' Minimal Angle:']
                   ]
                 ],

                 ['Tet', 'Tetrahedron',
                  [['QualityMeasureToEdgeRatio', ' Edge Ratio:'],
                   ['QualityMeasureToAspectRatio', ' Aspect Ratio:'],
                   ['QualityMeasureToRadiusRatio', ' Radius Ratio:'],
                   ['QualityMeasureToAspectFrobenius', ' Frobenius Norm:'],
                   ['QualityMeasureToMinAngle', ' Minimal Dihedral Angle:'],
                   ['QualityMeasureToCollapseRatio', ' Collapse Ratio:']
                   ]
                 ],

                 ['Hex', 'Hexahedron',
                  [['QualityMeasureToEdgeRatio', ' Edge Ratio:']
                   ]
                 ]

                ]

    if ug.GetNumberOfCells() > 0 :
        res = ''
        for meshType in meshTypes:
            if meshType[0] == 'Tet':
                res += '\n%s%s\n   %s' % ('Tetrahedral',
                       ' quality of the mesh:', mr.GetFileName())
            elif meshType[0] == 'Hex':
                res += '\n%s%s\n   %s' % ('Hexahedral',
                       ' quality of the mesh:', mr.GetFileName())
            else:
                res += '\n%s%s\n   %s' % (meshType[1],
                       ' quality of the mesh:', mr.GetFileName())

            for measure in meshType[2]:
                eval('iq.Set' + meshType[0] + measure[0] + '()')
                iq.Update()
                res += '\n%s\n%s' % (measure[1],
                       DumpQualityStats(iq, 'Mesh ' + meshType[1] + ' Quality'))

            res += '\n'

        print res

if __name__ == '__main__':
     main()
