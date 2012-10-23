/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkGhostArray.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkGhostArray.h -- Encodes/Decodes ghost array information.
//
// .SECTION Description
//  vtkGhostArray provides functionality for manipulating a mesh entity
//  property field, represented by an "unsigned char". Each mesh entity, e.g.,
//  a vertex or cell is associated with an "unsigned char" where each individual
//  bit represents the state of a particular property. This class provides
//  the logic required to manipulate individual bits in the "unsigned char".
//
// .SECTION Caveats
//  Since an unsigned char is used to represent a mesh entity property field, we
//  are restricted to at most 8 properties, i.e., [0-7] that can be used to
//  designate different states.

#ifndef vtkGhostArray_H_
#define vtkGhostArray_H_

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkObject.h"

#include <cassert> // For assert

class VTKFILTERSCORE_EXPORT vtkGhostArray : public vtkObject
{
public:

  enum
    {
    INTERNAL = 0, // Nodes that are on the interior domain of a partition
    SHARED   = 1, // Nodes that are on the abutting/internal interface of
                  // two or more partitions.
    GHOST    = 2, // Nodes whose value comes from another process/partition
    VOID     = 3, // Nodes that are ignored in computation/visualization,
                  // their value is typically garbage.
    IGNORE   = 4, // Nodes that are ignored in computation/visualization but
                  // have a valid value, e.g., if a SHARED node is going to be
                  // processed by another partition, then, this property is
                  // used to indicate to the rest of the partitions sharing
                  // that node to ignore it.
    BOUNDARY = 5, // Nodes that are on the boundaries of the domain
    PERIODIC = 6  // Nodes that are on periodic boundaries
    } NodeProperties;

  enum
    {
    DUPLICATE = 0, // Ghost cells that exist in another partition, i.e, are
                   // composed of internal boundary and/or ghost nodes
    EXTERNAL  = 1, // Cells that are created "artificially" outside the domain,
                   // i.e., are composed from boundary nodes and nodes outside
                   // the domain.
    BLANK     = 2, // Cells that are ignored in computation/visualization, their
                   // value is typically garbage, or in the case of AMR data,
                   // they have a value that is typically the average of the
                   // the values of each subdivision cell.
    INTERIOR  = 3  // Cells that are internal/owned by a given partition.
    } CellProperties;

  static vtkGhostArray* New();
  vtkTypeMacro( vtkGhostArray, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent );

  // Description:
  // Sets the given property in the propertyField.
  static void SetProperty(
      unsigned char &propertyField,const int property )
  {
    assert("pre:invalid property" && (property >= 0 && property < 8));
    propertyField |= (1 << property);
  }

  // Description:
  // Unsets the property from the given propertyField.
  static void UnsetProperty(
      unsigned char &propertyField,const int property )
  {
    assert("pre:invalid property" && (property >= 0 && property < 8));
    propertyField &= ~(1 << property);
  }

  // Description:
  // Checks if a property is set in the given property field.
  static bool IsPropertySet(
      unsigned char &propertyField,const int property )
  {
    assert("pre:invalid property" && (property >= 0 && property < 8));
    bool status = false;
    if( propertyField & (1 << property) )
      {
      status = true;
      }

    return status;
  }

  // Description:
  // Resets all the bits in the property field
  static void Reset( unsigned char &propertyField )
  {
    for( int i=0; i < 8; ++i )
      {
      vtkGhostArray::UnsetProperty( propertyField, i );
      }
  }

protected:
  vtkGhostArray();
  ~vtkGhostArray();

private:
  vtkGhostArray( const vtkGhostArray& ); // Not implemented
  void operator=(const vtkGhostArray& ); // Not implemented
};

#endif /* vtkGhostArray_H_ */
