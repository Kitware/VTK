/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredNeighbor.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkStructuredNeighbor.h -- Stores neighboring information
//
// .SECTION Description
//  An internal, light-weight class used to store neighbor information.

#ifndef VTKSTRUCTUREDNEIGHBOR_H_
#define VTKSTRUCTUREDNEIGHBOR_H_

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkObject.h"

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredNeighbor
{
public:

  // An enum that defines the neighboring orientation which is stored in a
  // 3-tuple vtkStructuredNeighbor::Orientation. In each dimension, there
  // is a high and low end, the orientation tuple defines how to grow ghost
  // layers along each dimension.
  enum NeighborOrientation
    {
    SUBSET_LO   = -2, // The grid is a subset of the neighboring grid and the
                      // ghost layers are pointing away from the low end
    LO          = -1, // The grid partially overlap with its neighbor on the
                      // low end, thus, ghost layers are pointing away from
                      // the low end
    ONE_TO_ONE  =  0, // grids abut 1-to-1 in both HI and LO, the
                      // cardinality of both grids is the same in the
                      // corresponding dimension.
    HI          =  1, // The grid partially overlaps with its neighbor on the
                      // high end, thus, ghost layers are pointing away from
                      // the high end
    SUBSET_HI   =  2, // The grid is a subset of the neighboring grid and the
                      // ghost layers are pointing away from the high end
    SUBSET_BOTH =  3, // The grid is a subset of the neighboring grid and the
                      // ghost layers grow from both low and high ends.
    SUPERSET    =  4, // grid is a superset of the neighboring grid in the
                      // given direction.
    UNDEFINED   =  5  // the neighboring relationship is undefined, e.g., if
                      // we are checking 2D data, the neighbor orientation
                      // in the 3rd dimension is undefined.
    };

  // Class Member Variables made public for easier access
  int NeighborID;       // The registered ID of the neighboring grid
  int OverlapExtent[6]; // The extent at which the grids overlap
  int SendExtent[6];    // The extent that we send to this neighbor
  int RcvExtent[6];     // The extent that we receive from this neighbor
  int Orientation[3];   // Defines how we are neighboring with this grid, see
                        // NeighborOrientation enum above.

  // Description:
  // Default Constructor
  vtkStructuredNeighbor();

  // Description:
  // Custom constructor. Constructs a neighbor with the prescribed neighbor
  // grid/block ID and overlap.
  vtkStructuredNeighbor( const int NeiID, int overlap[6] );

  // Description:
  // Custom constructor. Constructs a neighbor with the prescribed neigbhor
  // grid/block ID, overlap extent, and orientation
  vtkStructuredNeighbor( const int NeiID, int overlap[6], int orient[3] );

  // Description:
  // Copy constructor
  vtkStructuredNeighbor(const vtkStructuredNeighbor &N ){ *this = N; };

  // Description:
  // Default destructor
  virtual ~vtkStructuredNeighbor();


  // Description:
  // Overload assignment operator
  vtkStructuredNeighbor& operator=(const vtkStructuredNeighbor &N )
  {
    if( this != &N )
      {
      this->Orientation[ 0 ] = N.Orientation[ 0 ];
      this->Orientation[ 1 ] = N.Orientation[ 1 ];
      this->Orientation[ 2 ] = N.Orientation[ 2 ];
      this->NeighborID = N.NeighborID;
      for( int i=0; i < 6; ++i )
        {
        this->SendExtent[ i ]    = N.SendExtent[ i ];
        this->RcvExtent[ i ]     = N.RcvExtent[ i ];
        this->OverlapExtent[ i ] = N.OverlapExtent[ i ];
        } // END for
      } // END if
    return *this;
  }

  // Description:
  // Computes the SendExtent and the RcvExtent for this neighbor. The method
  // assumes that the overlap extent and orientation are already computed.
  // Using this information, the method grows the overlap extent to form the
  // Send and Rcv Extents for this neighbor instance.
  virtual void ComputeSendAndReceiveExtent(
      int gridRealExtent[6], int gridGhostedExtent[6], int neiRealExtent[6],
      int WholeExtent[6], const int N);
};

#endif /* VTKSTRUCTUREDNEIGHBOR_H_ */
// VTK-HeaderTest-Exclude: vtkStructuredNeighbor.h
