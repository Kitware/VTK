/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquaresCases.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkMarchingSquaresCases_h
#define __vtkMarchingSquaresCases_h
//
// marching squares cases for generating isolines
//

typedef int EDGE_LIST;
struct VTK_COMMON_EXPORT VTK_LINE_CASES 
{
  EDGE_LIST edges[5];
  static VTK_LINE_CASES* GetCases();
};

#endif
