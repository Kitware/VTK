/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContainers.cxx
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

#include "vtkObject.h"
#include "vtkAbstractList.h"
#include "vtkAbstractMap.h"

int main(int argc, char** vtkNotUsed(argv))
{
  // Let us for now just create pointers so that we see that
  // it can parse through the header file.
  vtkContainer *cnt = 0;
  vtkAbstractList<int> *alist = 0;
  vtkAbstractMap<char*, char*> *amap= 0;

  // This is here so that it does not complain about 
  // pointers not being used
  if ( cnt && alist && amap || (argc > 1) )
    {
    return 1;
    }
  return 0;
}
