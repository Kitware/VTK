/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiPartExtentTranslator.cxx
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
#include "vtkMultiPartExtentTranslator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiPartExtentTranslator, "1.1");
vtkStandardNewMacro(vtkMultiPartExtentTranslator);

//----------------------------------------------------------------------------
vtkMultiPartExtentTranslator::vtkMultiPartExtentTranslator()
{
}

//----------------------------------------------------------------------------
vtkMultiPartExtentTranslator::~vtkMultiPartExtentTranslator()
{
}

//----------------------------------------------------------------------------
int vtkMultiPartExtentTranslator::PieceToExtentThreadSafe(int , int , int , 
                                                          int *wholeExtent, 
                                                          int *resultExtent, 
                                                          int , int )
{
  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  return 1;
}


