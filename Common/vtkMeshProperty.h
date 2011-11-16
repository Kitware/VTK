/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGhostData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMeshProperty.h -- Defines node types and cell properties
//
// .SECTION Description
//  This header consists of the valid ghost node types and ghost cell types.

#ifndef VTKMESHPROPERTY_H_
#define VTKMESHPROPERTY_H_


namespace VTKNodeProperties {
  enum
  {
  INTERNAL = 0, // Nodes that are on the interior domain of a partition
  SHARED   = 1, // Nodes that are on the abutting/internal interface of two or
                // more partitions.
  GHOST    = 2, // Nodes whose value is coming from another process/partition
  VOID     = 3, // Nodes that are ignored in computation/visualization,
                // their value is typically garbage.
  IGNORE   = 4, // Nodes that are ignored in computation/visualization but have
                // a valid value, e.g., if a SHARED node is going to be processed
                // by another partition, then, this property is used to indicate
                // to the rest of the partitions sharing that node to ignore it.
  BOUNDARY = 5  // Nodes that are on the boundaries of the domain
  };
}

namespace VTKCellProperties {
  enum
  {
  DUPLICATE = 0,// Ghost cells that exist in another partition, i.e, are composed
                // of internal boundary and ghost nodes
  EXTERNAL  = 1 // Cells that are created "artificially" outside the domain, i.e.,
                // are composed from boundary nodes and nodes outside the domain.

  };
}
#endif /* VTKMESHPROPERTY_H_ */
