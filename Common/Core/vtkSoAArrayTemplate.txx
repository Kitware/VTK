/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSoAArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoAArrayTemplate<ScalarType>*
vtkSoAArrayTemplate<ScalarType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSoAArrayTemplate<ScalarType>);
}

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoAArrayTemplate<ScalarType>::vtkSoAArrayTemplate()
{
}

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoAArrayTemplate<ScalarType>::~vtkSoAArrayTemplate()
{
  this->Allocator.DeleteArray(this);
}
