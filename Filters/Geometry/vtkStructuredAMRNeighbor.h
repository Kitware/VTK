/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredAMRNeighbor.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkStructuredAMRNeighbor.h -- Stored AMR neighboring information
//
// .SECTION Description
//  An internal, light-weight object used to store neighbor information for
//  AMR grids.
//
// .SECTION See Also
//  vtkStructuredNeighbor vtkStructuredAMRGridConnectivity
#ifndef VTKSTRUCTUREDAMRNEIGHBOR_H_
#define VTKSTRUCTUREDAMRNEIGHBOR_H_

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkStructuredNeighbor.h"

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredAMRNeighbor :
  public vtkStructuredNeighbor
{
public:

  // An enum that defines the neighbor relationship between the 2 grids.
  enum NeighborRelationship
    {
    PARENT,                       // Neighbor fully contains this grid
    PARTIALLY_OVERLAPPING_PARENT, // Neighbor partially contains this grid
    CHILD,                        // This grid fully contains the neighbor
    PARTIALLY_OVERLAPPING_CHILD,  // This grid partially contains the neighbor
    SAME_LEVEL_SIBLING,           // Grids are adjacent at the same level
    COARSE_TO_FINE_SIBLING,       // Grid is adjacent with a finer neighbor
    FINE_TO_COARSE_SIBLING,       // Grid is adjacent with a coarser neighbor
    UNDEFINED
    };

  // NOTE: The OverlapExtent stores the overlap w.r.t. the neighboring grid
  // Consequently, GridOverlapExtent stores the overlap extent w.r.t. this grid.
  int GridOverlapExtent[6]; // The overlap extent w.r.t. this grid
  int GridLevel;      // The level of the grid that has this neighbor
  int NeighborLevel;  // The level of the neighboring grid
  int RelationShip;   // The relationship of the grid with this neighbor

  // Description:
  // Default constructor.
  vtkStructuredAMRNeighbor();

  // Description:
  // Custom constructor. Creates an AMR neighbor for a grid (block) at level
  // GridLevel with the neighboring block at NeiID, NeighborLevel. The two
  // neighbors overlap at the pre-computed overlap extent which is given w.r.t
  // to the current grid (i.e., not the neighboring grid).
  vtkStructuredAMRNeighbor(
     const int gridLevel,
     const int neiID, const int neighborLevel,
     int gridOverlap[6], int neiOverlap[6],
     int orient[3],
     const int relationShip);

  // Description:
  // Copy constructor.
  vtkStructuredAMRNeighbor(const vtkStructuredAMRNeighbor &N) :
    vtkStructuredNeighbor(N) { *this = N; }

  // Description:
  // Destructor.
  ~vtkStructuredAMRNeighbor() {}

  // Description:
  // Overload assignment operator.
  vtkStructuredAMRNeighbor& operator=(const vtkStructuredAMRNeighbor &N);

  // Description:
  // Returns the receive extent w.r.t. the grid's level, i.e., not the
  // neighbor's level.
  void GetReceiveExtentOnGrid(const int ng,int gridExtent[6],int ext[6]);

  // Description:
  // Returns the neighbor relationship as a string (usefule for debugging).
  std::string GetRelationShipString();

  // Description:
  // Computes the SendExtent and RcvExtent for this neighbor. The method assumes
  // that the overlap extent and orientation are already computed. Using this
  // information, the method grows the overlap extent to form the Send and Rcv
  // extents for this neighbor instance.
  virtual void ComputeSendAndReceiveExtent(
      int gridRealExtent[6], int gridGhostedExtent[6], int neiRealExtent[6],
      int WholeExtent[6], const int N);
};

#endif /* VTKSTRUCTUREDAMRNEIGHBOR_H_ */
// VTK-HeaderTest-Exclude: vtkStructuredAMRNeighbor.h
