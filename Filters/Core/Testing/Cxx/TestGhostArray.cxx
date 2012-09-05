/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGhostArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestGhostArray -- Tests vtkGhostArray
//
// .SECTION Description
//  A simple test for the vtkGhostArray

#include "vtkGhostArray.h"

int CheckSetAndUnsetProperty( unsigned char &u, const int p )
{
  int rc = 0;
  vtkGhostArray::SetProperty( u, p);
  if( !vtkGhostArray::IsPropertySet( u, p) )
    {
    std::cerr << "Could not set property " << p << std::endl;
    ++rc;
    }

  vtkGhostArray::UnsetProperty( u, p);
  if( vtkGhostArray::IsPropertySet( u, p) )
    {
    std::cerr << "Could not unset property " << p << std::endl;
    ++rc;
    }

  return( rc );
}

//------------------------------------------------------------------------------
int TestGhostArray(int,char *[])
{
  int rc = 0;
  unsigned char propertyField = '0';
  vtkGhostArray::Reset( propertyField );

  // Ensure all bits are set to 0
  for( int i=0; i < 8; ++i )
    {
    if( vtkGhostArray::IsPropertySet( propertyField, i) )
      {
      std::cerr << "bit " << i << " appears to be set!\n";
      ++rc;
      }
    } // END for

  // Try setting/unsetting some node properties
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::INTERNAL );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::SHARED );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::GHOST );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::IGNORE );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::VOID );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::BOUNDARY );

  // Try setting/unsetting some cell properties
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::DUPLICATE );
  rc += CheckSetAndUnsetProperty( propertyField, vtkGhostArray::EXTERNAL );

  // Ensure all bits are set to 0
  for( int i=0; i < 8; ++i )
    {
    if( vtkGhostArray::IsPropertySet( propertyField, i) )
      {
      std::cerr << "bit " << i << " appears to be set!\n";
      ++rc;
      }
    } // END for
  return( rc );
}
