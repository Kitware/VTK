/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMarkUtil - set of convenient helper functions for the Mark API.
//
// .SECTION Description
// 

#ifndef __vtkMarkUtil_h
#define __vtkMarkUtil_h

#include "vtkMark.h"

class VTK_CHARTS_EXPORT vtkMarkUtil
{
public:
  // Description:
  // Generate a default series of color varying with the index of the parent
  // of the given mark `m'. `d' is ignored.
  // It has the signature of a vtkValue::FunctionType.
  // It is useful with vtkBarMark of vtkLineMark to get a different color for
  // each set of bar or set of lines.
  static vtkColor DefaultSeriesColorFromParent(vtkMark* m, vtkDataElement&d);
  
  // Description:
  // Generate a default series of color varying with the index of the given
  // mark `m'. `d' is ignored.
  // It has the signature of a vtkValue::FunctionType.
  // It is useful with a vtkWedgeMark to get a different color for each
  // sector.
  static vtkColor DefaultSeriesColorFromIndex(vtkMark *m, vtkDataElement &d);
  
  static double StackLeft(vtkMark* m, vtkDataElement&)
  {
    return m->GetCousinLeft() + m->GetCousinWidth();
  }
  static double StackBottom(vtkMark* m, vtkDataElement&)
  {
    return m->GetCousinBottom() + m->GetCousinHeight();
  }
};

#endif // __vtkMarkUtil_h
