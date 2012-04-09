/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNonMergingPointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNonMergingPointLocator.h"

#include "vtkPoints.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkNonMergingPointLocator );

//----------------------------------------------------------------------------
int vtkNonMergingPointLocator::InsertUniquePoint
  ( const double x[3], vtkIdType & ptId )
{ 
  ptId = this->Points->InsertNextPoint( x ); 
  return 1; 
}

//----------------------------------------------------------------------------
void vtkNonMergingPointLocator::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );
}
