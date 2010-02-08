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

// .NAME vtkMarkUtil
//
// .SECTION Description
//

#ifndef __vtkMarkUtil_h
#define __vtkMarkUtil_h

#include "vtkMark.h"

class VTK_CHARTS_EXPORT vtkMarkUtil
{
public:
  static vtkColor DefaultSeriesColor(vtkMark* m, vtkDataElement&);
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
