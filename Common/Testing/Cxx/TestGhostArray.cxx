/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMeshPropertyEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestMeshPropertyEncoder.cxx -- Tests vtkMeshPropertyEncoder
//
// .SECTION Description
//  A simple test for the vtkMeshPropertyEncoder

#include "vtkMeshProperty.h"
#include "vtkMeshPropertyEncoder.h"

int CheckSetAndUnsetProperty( unsigned char &u, const int p )
{
  int rc = 0;
  vtkMeshPropertyEncoder::SetProperty( u, p);
  if( !vtkMeshPropertyEncoder::IsPropertySet( u, p) )
    {
    std::cerr << "Could not set property " << p << std::endl;
    ++rc;
    }

  vtkMeshPropertyEncoder::UnsetProperty( u, p);
  if( vtkMeshPropertyEncoder::IsPropertySet( u, p) )
    {
    std::cerr << "Could not unset property " << p << std::endl;
    ++rc;
    }

  return( rc );
}

//------------------------------------------------------------------------------
int TestMeshPropertyEncoder(int,char *[])
{
  int rc = 0;
  unsigned char propertyField;

  // Ensure all bits are set to 0
  for( int i=0; i < 8; ++i )
    {
    if( vtkMeshPropertyEncoder::IsPropertySet( propertyField, i) )
      ++rc;
    } // END for

  // Try setting/unsetting some node properties
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::INTERNAL );
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::SHARED );
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::GHOST );
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::IGNORE );
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::VOID );
  rc += CheckSetAndUnsetProperty( propertyField, VTKNodeProperties::BOUNDARY );

  // Try setting/unsetting some cell properties
  rc += CheckSetAndUnsetProperty( propertyField, VTKCellProperties::DUPLICATE );
  rc += CheckSetAndUnsetProperty( propertyField, VTKCellProperties::EXTERNAL );

  // Ensure all bits are set to 0
  for( int i=0; i < 8; ++i )
    {
    if( vtkMeshPropertyEncoder::IsPropertySet( propertyField, i) )
      ++rc;
    } // END for
  return( rc );
}
