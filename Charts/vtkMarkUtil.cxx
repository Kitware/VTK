/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMarkUtil.h"

#include "vtkPanelMark.h"

//-----------------------------------------------------------------------------
vtkColor vtkMarkUtil::DefaultSeriesColor(vtkMark* m, vtkDataElement&)
{
  unsigned char colors[10][3] = {{166, 206, 227},
                                 {31, 120, 180},
                                 {178, 223, 13},
                                 {51, 160, 44},
                                 {251, 154, 153},
                                 {227, 26, 28},
                                 {253, 191, 111},
                                 {255, 127, 0},
                                 {202, 178, 214},
                                 {106, 61, 154}};
  vtkIdType index = 0;
  if (m->GetParent())
    {
    index = m->GetParent()->GetIndex() % 10;
    }
  return vtkColor(colors[index][0]/255.0, colors[index][1]/255.0, colors[index][2]/255.0);
}
