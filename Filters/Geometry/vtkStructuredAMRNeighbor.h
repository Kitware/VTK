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
  int GridLevel;          // The level of the grid that has this neighbor.
  int NeighborLevel;      // The level of the neighboring grid
  int RefinementRatio[3]; // The refinement ratio, e.g., 2, 3, or 4.

  // Description:
  // Default constructor.
  vtkStructuredAMRNeighbor();

  // Description:
  // Custom constructor. Creates an AMR neighbor for a grid (block) at level
  // GridLevel with the neighboring block at NeiID, NeighborLevel. The two
  // neighbors overlap at the pre-computed overlap extent which is given w.r.t
  // to the current grid (i.e., not the neighboring grid).
  vtkStructuredAMRNeighbor(
     const int GridLevel, const int NeiID, const int NeighborLevel,
     int overlap[6], int orient[3], int r[3] );

  // Description:
  // Destructor.
  virtual ~vtkStructuredAMRNeighbor();

  // Description:
  // Overload assignment operator.
  vtkStructuredAMRNeighbor& operator=(const vtkStructuredAMRNeighbor &N);

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
