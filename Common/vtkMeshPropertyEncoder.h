/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkMeshPropertyEncoder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkMeshPropertyEncoder.h -- Encodes/Decodes mesh entity property info.
//
// .SECTION Description
//  vtkMeshPropertyEncoder provides functionality for manipulating a mesh entity
//  property field, represented by an "unsigned char". Each mesh entity, e.g.,
//  a vertex or cell is associated with an "unsigned char" where each individual
//  bit represents the state of a particular property. This class provides
//  the logic required to manipulate individual bits in the "unsigned char".
//
// .SECTION Caveats
//  Since a mesh property field is used to represent a mesh property field, we
//  are restricted to 8 properties that each property field can be associated
//  with.
//
// .SECTION See Also
//  vtkMeshProperty

#ifndef vtkMeshPropertyEncoder_H_
#define vtkMeshPropertyEncoder_H_

#include "vtkObject.h"

#include <cassert> // For assert

class VTK_COMMON_EXPORT vtkMeshPropertyEncoder : public vtkObject
{
  public:
    static vtkMeshPropertyEncoder* New();
    vtkTypeMacro( vtkMeshPropertyEncoder, vtkObject );
    void PrintSelf( std::ostream& os, vtkIndent indent );

    // Description:
    // Sets the given property in the propertyField.
    static void SetProperty( unsigned char &propertyField,const int property )
    {
      assert("pre:invalid property" && (property >= 0 && property < 8));
      propertyField |= (1 << property);
    }

    // Description:
    // Unsets the property from the given propertyField.
    static void UnsetProperty(unsigned char &propertyField,const int property )
    {
      assert("pre:invalid property" && (property >= 0 && property < 8));
      propertyField &= ~(1 << property);
    }

    // Description:
    // Checks if a property is set in the given property field.
    static bool IsPropertySet( unsigned char &propertyField,const int property )
    {
      assert("pre:invalid property" && (property >= 0 && property < 8));
      return( propertyField & (1 << property) );
    }

  protected:
    vtkMeshPropertyEncoder();
    ~vtkMeshPropertyEncoder();

  private:
    vtkMeshPropertyEncoder( const vtkMeshPropertyEncoder& ); // Not implemented
    void operator=(const vtkMeshPropertyEncoder& ); // Not implemented
};

#endif /* vtkMeshPropertyEncoder_H_ */
