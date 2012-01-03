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

#include "vtkObject.h"

class VTK_FILTERING_EXPORT vtkStructuredNeighbor
{
  public:

    // An enum that defines the neighboring orientation which is stored in a
    // 3-tuple vtkStructuredNeighbor::Orientation. In each dimension, a normal
    // to the neighbor can be drawn
    enum NeighborOrientation
      {
      LO          = -1, // normal to neighbor points away from the min
      ONE_TO_ONE  =  0, // grids abut 1-to-1 in both HI and LO, the
                        // cardinality of both grids is the same in the
                        // corresponding dimension.
      HI          =  1, // normal to neighbor points away from the max
      LO_SUBSET   =  2, // grids overlap in both HI,LO, normal points away
                        // from the LO of the neighboring grid.
      HI_SUBSET   =  3, // grids overlap in both HI,LO, normal points aways
                        // from the HI of the neighboring grid.
      SUBSET      =  4, // grids overlap in both HI,LO, the grid is a subset of
                        // of the neighboring grid which is strictly inclusive.
      SUPERSET    =  5, // grid is a superset of the neighboring grid in the
                        // given direction.
      UNDEFINED   =  6  // the neighboring relationship is undefined, e.g., if
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
    ~vtkStructuredNeighbor();


    // Description:
    // Overload assignement operator
    vtkStructuredNeighbor& operator=(const vtkStructuredNeighbor &N );

    // Description:
    // Flips the orientation of this neighbor.
    void FlipOrientation();

    // Description:
    // Computes the SendExtent and the RcvExtent for this neighbor. The method
    // assumes that the overlap extent and orientation are already computed.
    // Using this information, the method grows the overlap extent to form the
    // Send and Rcv Extents for this neighbor instance.
    void ComputeSendAndReceiveExtent( int WholeExtent[6], const int N);
};

//=============================================================================
//  INLINE METHODS
//=============================================================================

inline vtkStructuredNeighbor& vtkStructuredNeighbor::operator=(
    const vtkStructuredNeighbor &N )
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

//------------------------------------------------------------------------------
inline void vtkStructuredNeighbor::FlipOrientation()
{
  for( int i=0; i < 3; ++i )
    {
    switch( this->Orientation[i] )
      {
      case vtkStructuredNeighbor::LO:
        this->Orientation[ i ] = vtkStructuredNeighbor::HI;
        break;
      case vtkStructuredNeighbor::HI:
        this->Orientation[ i ] = vtkStructuredNeighbor::LO;
        break;
      case vtkStructuredNeighbor::LO_SUBSET:
        this->Orientation[ i ] = vtkStructuredNeighbor::LO;
        break;
      case vtkStructuredNeighbor::HI_SUBSET:
        this->Orientation[ i ] = vtkStructuredNeighbor::HI;
        break;
      case vtkStructuredNeighbor::SUBSET:
        this->Orientation[ i ] = vtkStructuredNeighbor::SUPERSET;
        break;
      case vtkStructuredNeighbor::SUPERSET:
        this->Orientation[ i ] = vtkStructuredNeighbor::SUBSET;
        break;
      default:
        ; // NO-OP do nothing
      } // END SWITCH
    } // END for all dimensions
}

#endif /* VTKSTRUCTUREDNEIGHBOR_H_ */
