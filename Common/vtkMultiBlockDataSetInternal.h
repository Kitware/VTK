/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSetInternal.h
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

#ifndef __vtkMultiBlockDataSetInternal_h
#define __vtkMultiBlockDataSetInternal_h

#include "vtkDataObject.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

class vtkMultiBlockDataSetInternal
{
public:
  vtkstd::vector< vtkSmartPointer<vtkDataObject> > DataSets;
};

#endif
